#include <giomm.h>
#include <glibmm.h>
#include <csignal>
#include <iostream>

const std::string BUS_NAME = "com.jimmyhodgson.wg";
const std::string OBJECT_PATH = "/com/jimmyhodgson/wg";
const std::string INTERFACE_NAME = "com.jimmyhodgson.wg";

std::unique_ptr<Gio::DBus::InterfaceVTable> vtable;

const char introspection_xml[] = R"XML(
<!DOCTYPE node PUBLIC
 "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="com.jimmyhodgson.wg">
    <method name="start">
        <arg type="a{sv}" name="response" direction="out"/>
    </method>
    <method name="stop">
        <arg type="a{sv}" name="response" direction="out"/>
    </method>
    <method name="status">
        <arg type="a{sv}" name="response" direction="out"/>
    </method>
  </interface>
</node>
)XML";


void on_method_call(
    const Glib::RefPtr<Gio::DBus::Connection>&,
    const Glib::ustring& sender,
    const Glib::ustring&,
    const Glib::ustring&,
    const Glib::ustring& method_name,
    const Glib::VariantContainerBase&,
    const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation)
{
    if (method_name == "start") {

        std::string output;
        std::string error;
        int exit_status;

        Glib::spawn_command_line_sync(
            "wg-quick up client",
            &output,
            &error,
            &exit_status
        );

        if(exit_status == 0) {
            std::map<Glib::ustring, Glib::VariantBase> dict = {
                { "status", Glib::Variant<bool>::create(true) },
                { "message", Glib::Variant<Glib::ustring>::create("Started the connection to the VPN") }
            };

            auto response_map = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(dict);

            auto response = Glib::VariantContainerBase::create_tuple({ response_map });
            invocation->return_value(response);
        } else {
            std::map<Glib::ustring, Glib::VariantBase> dict = {
                { "status", Glib::Variant<bool>::create(true) },
                { "message", Glib::Variant<Glib::ustring>::create("Error trying to start the VPN") },
                { "error", Glib::Variant<Glib::ustring>::create(error) }
            };

            auto response_map = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(dict);

            auto response = Glib::VariantContainerBase::create_tuple({ response_map });
            invocation->return_value(response);
        }
        
        
    }

    if (method_name == "stop") {

        std::string output;
        std::string error;
        int exit_status;

        Glib::spawn_command_line_sync(
            "wg-quick down client",
            &output,
            &error,
            &exit_status
        );

        if(exit_status == 0) {
            std::map<Glib::ustring, Glib::VariantBase> dict = {
                { "status", Glib::Variant<bool>::create(true) },
                { "message", Glib::Variant<Glib::ustring>::create("Stopped the connection to the VPN") }
            };

            auto response_map = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(dict);

            auto response = Glib::VariantContainerBase::create_tuple({ response_map });
            invocation->return_value(response);
        } else {
            std::map<Glib::ustring, Glib::VariantBase> dict = {
                { "status", Glib::Variant<bool>::create(false) },
                { "message", Glib::Variant<Glib::ustring>::create("Error trying to stop the VPN") },
                { "error", Glib::Variant<Glib::ustring>::create(error)}
            };

            auto response_map = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(dict);

            auto response = Glib::VariantContainerBase::create_tuple({ response_map });
            invocation->return_value(response);
        }
    }

    if (method_name == "status") {

        std::string output;
        std::string error;
        int exit_status;

        Glib::spawn_command_line_sync(
            "wg show",
            &output,
            &error,
            &exit_status
        );

        int status = !output.empty();
        std::string message = status ? "connected":"disconnected";

        std::map<Glib::ustring, Glib::VariantBase> dict = {
            { "status", Glib::Variant<bool>::create(status) },
            { "message", Glib::Variant<Glib::ustring>::create(message) }
        };

        auto response_map = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(dict);

        auto response = Glib::VariantContainerBase::create_tuple({ response_map });
        invocation->return_value(response);
    }
}

bool on_interface_set_property(const Glib::RefPtr<Gio::DBus::Connection>&
connection, const Glib::ustring& sender, const Glib::ustring&
object_path, const Glib::ustring& interface_name, const Glib::ustring&
property_name, const Glib::VariantBase& value) {
    return true;
}

void on_interface_get_property(Glib::VariantBase& property, const
Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring&
sender, const Glib::ustring& object_path, const Glib::ustring&
interface_name, const Glib::ustring& property_name) {
    property = Glib::Variant<Glib::ustring>::create("");
}

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& name) {
    auto introspection_data = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);
    auto interface_info = introspection_data->lookup_interface(INTERFACE_NAME);

    connection->register_object(
        OBJECT_PATH,
        interface_info,
        *vtable
    );
}

int main() {
    Gio::init();
    auto loop = Glib::MainLoop::create();

    auto onBusAcquired = Gio::DBus::SlotBusAcquired(&on_bus_acquired);

    auto methodCall = Gio::DBus::InterfaceVTable::SlotInterfaceMethodCall(&on_method_call);
    auto getProperty = Gio::DBus::InterfaceVTable::SlotInterfaceGetProperty(&on_interface_get_property);
    auto setProperty = Gio::DBus::InterfaceVTable::SlotInterfaceSetProperty(&on_interface_set_property);
    
    vtable = std::make_unique<Gio::DBus::InterfaceVTable>(methodCall, getProperty, setProperty);

    Gio::DBus::own_name(
        Gio::DBus::BUS_TYPE_SYSTEM,
        BUS_NAME,
        // on_bus_acquired
        onBusAcquired,
        // on_name_acquired
        [](const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring& name) {
            std::cout << "Acquired name: " << name << std::endl;
        },
        // on_name_lost
        [](const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring& name) {
            std::cerr << "Lost name: " << name << std::endl;
        }
    );

    loop->run();
    return 0;
}
