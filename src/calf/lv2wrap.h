#if USE_LV2

#include <string>
#include <lv2.h>
#include <calf/giface.h>

namespace synth {

template<class Module>
struct lv2_instance: public Module, public plugin_ctl_iface
{
    lv2_instance()
    {
        for (int i=0; i < Module::in_count; i++)
            Module::ins[i] = NULL;
        for (int i=0; i < Module::out_count; i++)
            Module::outs[i] = NULL;
        for (int i=0; i < Module::param_count; i++)
            Module::params[i] = NULL;
    }
    virtual parameter_properties *get_param_props(int param_no)
    {
        return &Module::param_props[param_no];
    }
    virtual float get_param_value(int param_no)
    {
        return *Module::params[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        *Module::params[param_no] = value;
    }
    virtual int get_param_count()
    {
        return Module::param_count;
    }
    virtual int get_param_port_offset() 
    {
        return Module::in_count + Module::out_count;
    }
    virtual const char *get_gui_xml() {
        return Module::get_gui_xml();
    }
    virtual line_graph_iface *get_line_graph_iface()
    {
        return this;
    }
    virtual bool activate_preset(int bank, int program) { 
        return false;
    }
};

template<class Module>
struct lv2_wrapper
{
    typedef lv2_instance<Module> instance;
    static LV2_Descriptor descriptor;
    std::string uri;
    
    lv2_wrapper(ladspa_info &info)
    {
        uri = "http://calf.sourceforge.net/plugins/" + std::string(info.label);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        unsigned long params = Module::param_count;
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
    }

    static void cb_activate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->set_sample_rate(mod->srate);
        mod->activate();
    }
    
    static void cb_deactivate(LADSPA_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance();
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->srate = (uint32_t)sample_rate;
        return mod;
    }
    static inline void zero_by_mask(Module *module, uint32_t mask, uint32_t offset, uint32_t nsamples)
    {
        for (int i=0; i<Module::out_count; i++) {
            if ((mask & (1 << i)) == 0) {
                dsp::zero(module->outs[i] + offset, nsamples);
            }
        }
    }
    static inline void process_slice(Module *mod, uint32_t offset, uint32_t end)
    {
        while(offset < end)
        {
            uint32_t newend = std::min(offset + MAX_SAMPLE_RUN, end);
            uint32_t out_mask = mod->process(offset, newend - offset, -1, -1);
            zero_by_mask(mod, out_mask, offset, newend - offset);
            offset = newend;
        }
    }

    static void cb_run(LV2_Handle Instance, uint32_t SampleCount) {
        instance *const mod = (instance *)Instance;
        mod->params_changed();
        process_slice(mod, 0, SampleCount);
    }
    static void cb_cleanup(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    static const void *cb_ext_data(const char *URI) {
        return NULL;
    }
};

template<class Module>
LV2_Descriptor lv2_wrapper<Module>::descriptor;

};

#endif
