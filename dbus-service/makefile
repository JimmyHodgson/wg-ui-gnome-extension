PROJ_DOMAIN := com.jimmyhodgson.wg
DBUS_CONF_DIR := /etc/dbus-1/system.d
DBUS_SERVER_DIR := /usr/local/bin/
DBUS_SERVICE_DIR := /usr/share/dbus-1/system-services

DBUS_CONF_FILE := $(DBUS_CONF_DIR)/$(PROJ_DOMAIN).conf
DBUS_SERVER_FILE := $(DBUS_SERVER_DIR)/dbus-server
DBUS_SERVICE_FILE := $(DBUS_SERVICE_DIR)/$(PROJ_DOMAIN).service

dbus-server: main.cpp
	@g++ -std=c++17 main.cpp -o dbus-server `pkg-config --cflags --libs \
	giomm-2.4`

.PHONY: build
build: dbus-server

.PHONY: clean
clean:
	@rm dbus-server

.PHONY: install
install: $(DBUS_SERVER_FILE) $(DBUS_CONF_FILE) $(DBUS_SERVICE_FILE)

$(DBUS_SERVER_FILE): dbus-server
	@sudo pkill dbus-server || true
	@sudo cp dbus-server $@

$(DBUS_SERVICE_FILE): $(PROJ_DOMAIN).service
	@sudo cp $(PROJ_DOMAIN).service $@

$(DBUS_CONF_FILE): $(PROJ_DOMAIN).conf
	@sudo cp $(PROJ_DOMAIN).conf $@