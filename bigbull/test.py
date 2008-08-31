#!/usr/bin/env python
import calfpytools
import time

print calfpytools.scan_ttl_file("/usr/local/lib/lv2/allpass-swh.lv2/plugin.ttl")

client = calfpytools.JackClient()
client.open("calf")
print client.get_cobj()
port = client.register_port("port")
print port
print port.get_cobj()
assert port.get_name() == "port"
assert port.get_full_name() == "calf:port"
assert port.set_name("port2") == "port2"
assert port.get_name() == "port2"
assert port.get_full_name() == "calf:port2"
assert port.is_valid()

# This doesn't work: assert client.get_port("calf:port2") == port (because JACK C API doesn't reuse the jack_port_t structs)

port2 = client.get_port("system:playback_1")
assert port2.get_name() == "playback_1"
print port2.get_full_name()
print port2.get_aliases()
# prevent Patchage from crashing
time.sleep(1)
port.unregister()
assert port2 == client.get_port("system:playback_1")
assert not port.is_valid()
print port2.get_connections()
#client.close()

