# Wireguard UI Gnome Extension

A simple wireguard UI built for gnome 43.

## dbus-service

This project uses wg-quick to start and stop the wireguard vpn connection.
Since wg-quick needs to run with sudo privileges a dbus service was created to
provide a simple api to control the status of the vpn.

### Assumptions

* Wireguard is installed
* A configuration file named client.conf exists at `/etc/wireguard/client.conf`

### API

The service supports the following commands:

| Method | Command              | Description                                      |
| ------ | -------------------- | ------------------------------------------------ |
| status | wg show              | Returns the current connection status of the vpn |
| start  | wg-quick up client   | Starts the vpn connection                        |
| stop   | wg-quick down client | Stops the vpn connection                         |

### Testing the API

Once the dbus service is installed you can test it by sending messages from the
terminal with the following command:

```shell
gdbus call --system --dest com.jimmyhodgson.wg \
--object-path /com/jimmyhodgson/wg --method com.jimmyhodgson.wg.status
```

Where:

* `--dest com.jimmyhodgson.wg` specifies the name of the bus to query.
* `--object-path /com/jimmyhodgson/wg` specifies the exposed object by the bus.
* `--method com.jimmyhodgson.wg.status` specifies the method to call. In this
case `status`.

### Response format

The dbus-service returns responses as a key-value dictionary with the following fields:

| key     | Value  | Description                                                  | Required |
| ------- | ------ | ------------------------------------------------------------ | -------- |
| status  | bool   | Status of the operation, true for success, false on failure. | Yes      |
| message | string | Description of the result                                    | Yes      |
| error   | string | Output of the error                                          | No       |

#### Example Response

```json
{
    "status": false,
    "message": "Error trying to stop the VPN",
    "error": "wg-quick: `client' is not a WireGuard interface"
}
```
## gnome extension

This extension provides a simple wireguard button that shows up in the gnome
status bar.

![WG Gnome extension UI](extension.png)

On click it will connect or disconnect the VPN. When connected, the icon will
turn red. It will stay white when in the disconnect state.

On a critical error, the icon will turn black.

### Testing
On wayland, you can easily test an extension by spawning a nested shell with
the following command:

```shell
dbus-run-session -- gnome-shell --nested --wayland
```