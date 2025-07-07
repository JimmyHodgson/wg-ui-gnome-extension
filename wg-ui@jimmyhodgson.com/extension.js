const ExtensionUtils = imports.misc.extensionUtils;
const Me = ExtensionUtils.getCurrentExtension();
const { St, Clutter, Gio } = imports.gi;
const Main = imports.ui.main;

const PanelMenu = imports.ui.panelMenu;
const PopupMenu = imports.ui.popupMenu;

const GLib = imports.gi.GLib;

function sendNotification(icon, message) {
    try {
        // Create proxy
            let proxy = new Gio.DBusProxy.new_for_bus_sync(
                Gio.BusType.SESSION,
                Gio.DBusProxyFlags.NONE,
                null,
                'org.freedesktop.Notifications',
                '/org/freedesktop/Notifications',
                'org.freedesktop.Notifications',
                null
            );

            proxy.call(
                "Notify",
                new GLib.Variant("(susssasa{sv}i)",
                    [
                        'WireguardUI',  // s: app_name
                        0,              // u: replaces_id
                        icon,           // s: app_icon
                        'Wireguard UI', // s: summary
                        message,        // s: body
                        [],             // as: actions
                        {},             // a{sv}: hints
                        -1              // i: expire_timeout
                    ]
                ),
                Gio.DBusCallFlags.NONE,
                -1,
                null,
                (proxy, res) => {}
            );
    } catch(e){
        logError(e,"Unable to send notification");
    }
}

/**
 * 
 * @param {string} method name of the method to call 
 * @returns null on error, dictionary variant on success.
 *          Note that deep_unpack() needs to be called on the properties of this
 *          dictionary to be able to access their value.
 */
function callService(method) {
    log(`calling wg dbus service for method: ${method}`)
    return new Promise((resolve) => {
        try {
            // Create proxy
            let proxy = new Gio.DBusProxy.new_for_bus_sync(
                Gio.BusType.SYSTEM,
                Gio.DBusProxyFlags.NONE,
                null,
                'com.jimmyhodgson.wg',
                '/com/jimmyhodgson/wg',
                'com.jimmyhodgson.wg',
                null
            );

            // Call the method
            proxy.call(
                method,
                null,
                Gio.DBusCallFlags.NONE,
                -1,
                null,
                (proxy, res) => {
                    let result = proxy.call_finish(res);
                    let [dictVariant] = result.deep_unpack();
                    resolve(dictVariant);
                }
            );

        } catch (e) {
            logError(e, 'Failed to call status');
            resolve(null);
        }
    });
}

class Extension {

    constructor() {
        log(`constructing ${Me.metadata.name}`);
        this.stylesheet = Gio.File.new_for_path(Me.path + '/stylesheet.css');
        this.iconFile = Gio.File.new_for_path(Me.path + '/wg-ui-symbolic.svg');
        this.gioIcon = new Gio.FileIcon({ file: this.iconFile });

        // binding `this` to our callbacks so context isn't lost
        this.handleOnClick = this.handleOnClick.bind(this);
        this.handleError = this.handleError.bind(this);
        this.handleStart = this.handleStart.bind(this);
        this.handleStop = this.handleStop.bind(this);
    }

    /**
     * This function is called when your extension is enabled, which could be
     * done in GNOME Extensions, when you log in or when the screen is unlocked.
     *
     * This is when you should setup any UI for your extension, change existing
     * widgets, connect signals or modify GNOME Shell's behavior.
     */
    enable() {
        let initialColor = "logo-white";

        callService("status").then(initialResult => {
            if (!initialResult) {
                log("Unable to call the dbus service helper");
                initialColor = "logo-black";
                return;
            }

            initialColor
                = initialResult.status.deep_unpack() ? 'logo-red'
                    : 'logo-white';

            log(`initial result is = ${initialResult.message.deep_unpack()}`);

            St.ThemeContext.get_for_stage(global.stage).get_theme()
                .load_stylesheet(this.stylesheet);

            this.panelButton
                = new PanelMenu.Button(0.0, _("WireguardUI"), false);

            let box = new St.BoxLayout();
            this.icon = new St.Icon({
                gicon: this.gioIcon,
                style_class: `system-status-icon ${initialColor}`
            });

            box.add_child(this.icon);
            this.panelButton.add_child(box);

            this.panelButton
                .connect('button-press-event', this.handleOnClick);

            // Add the button to the panel
            Main.panel
                .addToStatusArea('WireguardUI', this.panelButton, 0, 'right');
        });
    }


    /**
     * This function is called when your extension is uninstalled, disabled in
     * GNOME Extensions or when the screen locks.
     *
     * Anything you created, modified or setup in enable() MUST be undone here.
     * Not doing so is the most common reason extensions are rejected in review!
     */
    disable() {
        log(`disabling ${Me.metadata.name}`);
        this.panelButton.destroy();
    }

    handleError() {
        log("Handling Error");
        this.icon.remove_style_class_name('logo-red');
        this.icon.remove_style_class_name('logo-white');
        this.icon.add_style_class_name('logo-black');
    }

    handleStart(result) {
        log("Handling start");
        if (!result) {
            return this.handleError();
        }

        if (result.status.deep_unpack()) {
            this.icon.remove_style_class_name('logo-white');
            this.icon.add_style_class_name('logo-red');

            sendNotification('network-vpn',"Connected");
        } else {
            log(result.error.deep_unpack());
        }
    }

    handleStop(result) {
        log("Handling stop");
        if (!result) {
            return this.handleError();
        }

        if (result.status.deep_unpack()) {
            this.icon.remove_style_class_name('logo-red');
            this.icon.add_style_class_name('logo-white');
            sendNotification('network-vpn-disconnected',"Disconnected");
        } else {
            log(result.error.deep_unpack());
        }
    }

    handleOnClick() {
        log("Handling extension click");
        callService("status").then(currentStatus => {
            if (!currentStatus) {
                return this.handleError();
            }

            if (currentStatus.status.deep_unpack()) {
                // vpn is active
                callService("stop").then(this.handleStop)
                    .catch(e => logError(e, 'Failed to call service'));
            } else {
                // vpn is inactive
                callService("start").then(this.handleStart)
                    .catch(e => logError(e, 'Failed to call service'));
            }
        });
    }
}

/**
 * This function is called once when your extension is loaded, not enabled. This
 * is a good time to setup translations or anything else you only do once.
 *
 * You MUST NOT make any changes to GNOME Shell, connect any signals or add any
 * MainLoop sources here.
 *
 * @param {ExtensionMeta} meta - An extension meta object
 * @returns {object} an object with enable() and disable() methods
 */
function init(meta) {
    console.debug(`initializing ${meta.metadata.name}`);

    return new Extension();
}
