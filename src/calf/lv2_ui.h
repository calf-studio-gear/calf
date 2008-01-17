/************************************************************************
 *
 * In-process UI extension for LV2
 *
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * 
 * Based on lv2.h, which was
 *
 * Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, 
 *                         Stefan Westerfeld
 * Copyright (C) 2006 Steve Harris, Dave Robillard.
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 * USA.
 *
 ***********************************************************************/

/** @file
    This extension defines an interface that can be used in LV2 plugins and
    hosts to create UIs for plugins. The UIs are plugins that reside in
    shared object files in an LV2 bundle and are referenced in the RDF data
    using the triples (Turtle shown)
<pre>    
    @@prefix uiext: <http://ll-plugins.nongnu.org/lv2/ext/ui#> .
    <http://my.plugin>    uiext:ui     <http://my.pluginui> .
    <http://my.plugin>    a            uiext:GtkUI .
    <http://my.pluginui>  uiext:binary <myui.so> .
</pre>
    where <http://my.plugin> is the URI of the plugin, <http://my.pluginui> is
    the URI of the plugin UI and <myui.so> is the relative URI to the shared 
    object file. While it is possible to have the plugin UI and the plugin in 
    the same shared object file it is probably a good idea to keep them 
    separate so that hosts that don't want UIs don't have to load the UI code.
    A UI MUST specify its class in the RDF data. In this case the class is
    uiext:GtkUI, which is the only class defined by this extension.
    
    (Note: the prefix above is used throughout this file for the same URI)
    
    It's entirely possible to have multiple UIs for the same plugin, or to have
    the UI for a plugin in a different bundle from the actual plugin - this
    way people other than the plugin author can write plugin UIs independently
    without editing the original plugin bundle.
    
    Note that the process that loads the shared object file containing the UI
    code and the process that loads the shared object file containing the 
    actual plugin implementation does not have to be the same. There are many
    valid reasons for having the plugin and the UI in different processes, or
    even on different machines. This means that you can _not_ use singletons
    and global variables and expect them to refer to the same objects in the
    UI and the actual plugin. The function callback interface defined in this
    header is all you can expect to work.
    
    Since the LV2 specification itself allows for extensions that may add 
    new types of data and configuration parameters that plugin authors may 
    want to control with a UI, this extension allows for meta-extensions that
    can extend the interface between the UI and the host. These extensions
    mirror the extensions used for plugins - there are required and optional
    "features" that you declare in the RDF data for the UI as
<pre>    
    <http://my.pluginui> uiext:requiredFeature <http://my.feature> .
    <http://my.pluginui> uiext:optionalFeature <http://my.feature> .
</pre>
    These predicates have the same semantics as lv2:requiredFeature and 
    lv2:optionalFeature - if a UI is declaring a feature as required, the
    host is NOT allowed to load it unless it supports that feature, and if it
    does support a feature (required or optional) it MUST pass that feature's
    URI and any additional data (specified by the meta-extension that defines
    the feature) to the UI's instantiate() function.
    
    These features may be used to specify how to pass data between the UI
    and the plugin port buffers - see LV2UI_Write_Function for details.
    
    UIs written to this specification do not need to be threadsafe - the 
    functions defined below may only be called in the same thread as the UI
    main loop is running in.
    
    Note that this UI extension is NOT a lv2:Feature. There is no way for a 
    plugin to know whether the host that loads it supports UIs or not, and 
    the plugin must ALWAYS work without the UI (although it may be rather 
    useless unless it has been configured using the UI in a previous session).
    
    A UI does not have to be a graphical widget, it could just as well be a
    server listening for OSC input or an interface to some sort of hardware
    device, depending on the RDF class of the UI.
*/

#ifndef LV2_IPUI_H
#define LV2_IPUI_H

#include "lv2.h"


#ifdef __cplusplus
extern "C" {
#endif


/** A pointer to some widget or other type of UI handle.
    The actual type is defined by the type URI of the UI, e.g. if 
    "<http://example.org/someui> a uiext:GtkUI", this is a pointer
    to a GtkWidget compatible with GTK+ 2.0 and the UI can expect the GTK+
    main loop to be running during the entire lifetime of all instances of that
    UI. All the functionality provided by this extension is toolkit 
    independent, the host only needs to pass the necessary callbacks and 
    display the widget, if possible. Plugins may have several UIs, in various
    toolkits, but uiext:GtkUI is the only type that is defined in this 
    extension. */
typedef void* LV2UI_Widget;


/** A pointer to some host data required to instantiate a UI.
    Like the type of the widget, the actual type of this pointer is defined by
    the type URI of the UI.  Hosts can use this to pass toolkit specific data
    to a UI it needs to instantiate (type map, drawing context, etc). For the
    uiext:GtkUI type this should be NULL. */
typedef void* LV2UI_Host_Data;
	
  
/** This handle indicates a particular instance of a UI.
    It is valid to compare this to NULL (0 for C++) but otherwise the 
    host MUST not attempt to interpret it. The UI plugin may use it to 
    reference internal instance data. */
typedef void* LV2UI_Handle;


/** This handle indicates a particular plugin instance, provided by the host.
    It is valid to compare this to NULL (0 for C++) but otherwise the 
    UI plugin MUST not attempt to interpret it. The host may use it to 
    reference internal plugin instance data. */
typedef void* LV2UI_Controller;


/** This is the type of the host-provided function that the UI can use to
    send data to a plugin's input ports. The @c buffer parameter must point
    to a block of data, @c buffer_size bytes large. The contents of this buffer
    will depend on the class of the port it's being sent to, and the transfer
    mechanism specified for that port class. 
    
    Transfer mechanisms are Features and may be defined in meta-extensions. 
    They specify how to translate the data buffers passed to this function 
    to input data for the plugin ports. If a UI wishes to write data to an 
    input port, it must list a transfer mechanism Feature for that port's 
    class as an optional or required feature (depending on whether the UI 
    will work without being able to write to that port or not). The only 
    exception is ports of the class lv2:ControlPort, for which @c buffer_size
    should always be 4 and the buffer should always contain a single IEEE-754
    float.
    
    The UI MUST NOT try to write to a port for which there is no specified
    transfer mechanism, or to an output port. The UI is responsible for 
    allocating the buffer and deallocating it after the call. A function 
    pointer of this type will be provided to the UI by the host in the 
    instantiate() function. */
typedef void (*LV2UI_Write_Function)(LV2UI_Controller controller,
                                     uint32_t         port_index,
                                     uint32_t         buffer_size,
                                     const void*      buffer);


/** */
typedef struct _LV2UI_Descriptor {
  
  /** The URI for this UI (not for the plugin it controls). */
  const char* URI;
  
  /** Create a new UI object and return a handle to it. This function works
      similarly to the instantiate() member in LV2_Descriptor.
      
      @param descriptor The descriptor for the UI that you want to instantiate.
      @param plugin_uri The URI of the plugin that this UI will control.
      @param bundle_path The path to the bundle containing the RDF data file
                         that references this shared object file, including the
                         trailing '/'.
      @param write_function A function provided by the host that the UI can
                            use to send data to the plugin's input ports.
      @param controller A handle for the plugin instance that should be passed
                        as the first parameter of @c write_function.
      @param host_data  Data required from the host for instantiation.
                        The type of this depends on the RDF class of the UI.
			If the UI type does not specify anything to be passed
			here, the host should pass NULL.
      @param widget     A pointer to an LV2UI_Widget. The UI will write a
                        widget pointer to this location (what type of widget 
                        depends on the RDF class of the UI) that will be the
                        main UI widget.
      @param features   An array of LV2_Feature pointers. The host must pass
                        all feature URIs that it and the plugin supports and any
                        additional data, just like in the LV2 plugin 
                        instantiate() function.
  */
  LV2UI_Handle (*instantiate)(const struct _LV2UI_Descriptor* descriptor,
                              const char*                     plugin_uri,
                              const char*                     bundle_path,
                              LV2UI_Write_Function            write_function,
                              LV2UI_Controller                controller,
                              LV2UI_Host_Data                 host_data,
                              LV2UI_Widget*                   widget,
                              const LV2_Feature* const*       features);

  
  /** Destroy the UI object and the associated widget. The host must not try
      to access the widget after calling this function.
   */
  void (*cleanup)(LV2UI_Handle ui);
  
  /** Tell the UI that something interesting has happened at a plugin port.
      What is interesting and how it is written to the buffer passed to this
      function is defined by the specified transfer mechanism for that port 
      class (see LV2UI_Write_Function). The only exception is ports of the 
      class lv2:ControlPort, for which this function should be called
      when the port value changes (it must not be called for every single 
      change if the host's UI thread has problems keeping up with the thread
      the plugin is running in), @c buffer_size should be 4 and the buffer
      should contain a single IEEE-754 float.
      
      By default, the host should only call this function for input ports of
      the lv2:ControlPort class. However, the default setting can be modified
      by using the following URIs in the UI's RDF data:
      <pre>
      uiext:portNotification
      uiext:noPortNotification
      uiext:plugin
      uiext:portIndex
      </pre>
      For example, if you want the UI with uri 
      <code><http://my.pluginui></code> for the plugin with URI 
      <code><http://my.plugin></code> to get notified when the value of the 
      output control port with index 4 changes, you would use the following 
      in the RDF for your UI:
      <pre>
      <http://my.pluginui> uiext:portNotification [ uiext:plugin <http://my.plugin> ;
                                                      uiext:portIndex 4 ] .
      </pre>
      and similarly with <code>uiext:noPortNotification</code> if you wanted
      to prevent notifications for a port for which it would be on by default 
      otherwise. The UI is not allowed to request notifications for ports
      for which no transfer mechanism is specified, if it does it should be
      considered broken and the host should not load it.
      
      The @c buffer is only valid during the time of this function call, so if 
      the UI wants to keep it for later use it has to copy the contents to an
      internal buffer.
      
      This member may be set to NULL if the UI is not interested in any 
      port events.
  */
  void (*port_event)(LV2UI_Handle ui,
                     uint32_t     port,
                     uint32_t     buffer_size,
                     const void*  buffer);
  
  /** Returns a data structure associated with an extension URI, for example
      a struct containing additional function pointers. Avoid returning
      function pointers directly since standard C++ has no valid way of
      casting a void* to a function pointer. This member may be set to NULL
      if the UI is not interested in supporting any extensions. This is similar
      to the extension_data() member in LV2_Descriptor.
  */
  const void* (*extension_data)(const char*  uri);

} LV2UI_Descriptor;



/** A plugin UI programmer must include a function called "lv2ui_descriptor"
    with the following function prototype within the shared object
    file. This function will have C-style linkage (if you are using
    C++ this is taken care of by the 'extern "C"' clause at the top of
    the file). This function will be accessed by the UI host using the 
    @c dlsym() function and called to get a LV2UI_UIDescriptor for the
    wanted plugin.
    
    Just like lv2_descriptor(), this function takes an index parameter. The
    index should only be used for enumeration and not as any sort of ID number -
    the host should just iterate from 0 and upwards until the function returns
    NULL, or a descriptor with an URI matching the one the host is looking for
    is returned.
*/
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index);


/** This is the type of the lv2ui_descriptor() function. */
typedef const LV2UI_Descriptor* (*LV2UI_DescriptorFunction)(uint32_t index);



#ifdef __cplusplus
}
#endif


#endif
