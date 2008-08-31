#!/usr/bin/env python
import calfpytools

client = calfpytools.JackClient()
client.open("calf")
port = client.register_port("port")
print port
assert port.get_name() == "port"
assert port.get_full_name() == "calf:port"
assert port.set_name("port2") == "port2"
assert port.get_name() == "port2"
assert port.get_full_name() == "calf:port2"
assert port.is_valid()
port2 = client.get_port("system:playback_1")
assert port2.get_name() == "playback_1"
print port2.get_full_name()
print port2.get_aliases()
port.unregister()
assert not port.is_valid()
client.close()
