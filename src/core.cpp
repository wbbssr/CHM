#include "core.h"

core::core()
{
    BOOST_LOG_FUNCTION();

    //default logging level
    _log_level = debug;

    _log_sink = boost::make_shared< text_sink >();
    {
        text_sink::locked_backend_ptr pBackend = _log_sink->locked_backend();

        #if (BOOST_VERSION / 100 % 1000) < 56
        boost::shared_ptr< std::ostream > pStream(&std::clog,  logging::empty_deleter());
        #else
        boost::shared_ptr< std::ostream > pStream(&std::clog,  boost::null_deleter());
        #endif
        pBackend->add_stream(pStream);


        boost::shared_ptr< std::ofstream > pStream2(new std::ofstream("CHM.log"));

        if (!pStream2->is_open())
        {
            BOOST_THROW_EXCEPTION(file_write_error()
                    << boost::errinfo_errno(errno)
                    << boost::errinfo_file_name("CHM.log")
                    );
        }

        pBackend->add_stream(pStream2);
    }

    _log_sink->set_formatter
            (
            expr::format("%1% %2% [%3%]: %4%")
            % expr::attr< boost::posix_time::ptime >("TimeStamp")
            % expr::format_named_scope("Scope",
            keywords::format = "%n:%l",
            keywords::iteration = expr::reverse,
            keywords::depth = 1)
            % expr::attr< log_level>("Severity")
            % expr::smessage
            );

    logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
    logging::core::get()->add_global_attribute("Scope", attrs::named_scope());

    logging::core::get()->add_sink(_log_sink);

    LOG_DEBUG << "Logger initialized. Writing to cout and CHM.log";

#ifdef NOMATLAB
    _engine = boost::make_shared<maw::matlab_engine>();


    _engine->start();
    _engine->set_working_dir();

    LOG_DEBUG << "Matlab engine started";
#endif

    _global = boost::make_shared<global>();

    //don't just abort and die
    gsl_set_error_handler_off();
}

core::~core()
{
    LOG_DEBUG << "Finished";
}

void core::config_debug(const pt::ptree& value)
{
    LOG_DEBUG << "Found debug section";
    std::string s = value.get("debug_level","debug");

    if (s == "debug")
        _log_level = debug;
    else if (s == "warning")
        _log_level = warning;
    else if (s == "error")
        _log_level = error;
    else if (s == "verbose")
        _log_level = verbose;

    LOG_DEBUG << "Setting log severity to " << _log_level;

    _log_sink->set_filter(
            severity >= _log_level
            );

}

void core::config_modules(const pt::ptree& value,const pt::ptree& config)
{
    LOG_DEBUG << "Found modules section";
    int modnum = 0;
    //loop over the list of requested modules
    // these are in the format "type":"ID"
    for (auto& itr : value)
    {

        std::string module = itr.second.data();
        LOG_DEBUG << "Module ID=" << module;


        //try grabbing a config for this module, empty string default
        pt::ptree cfg;

        try
        {
            cfg=config.get_child(module);
        }catch(pt::ptree_bad_path& e)
        {
            LOG_DEBUG << "No config for " << module;
        }
        boost::shared_ptr<module_base> m(_mfactory.get(module,cfg));
        //internal tracking of module initialization order

        m->IDnum = modnum;
        modnum++;
        _modules.push_back(std::make_pair(m, 1)); //default to 1 for make ordering, we will set it later in determine_module_dep
    }

    if (modnum == 0)
    {
        BOOST_THROW_EXCEPTION(no_modules_defined() << errstr_info("No modules defined. Aborting"));
    }
}

void core::config_forcing(const pt::ptree& value)
{
    LOG_DEBUG << "Found forcing section";
    //loop over the list of forcing data
    for (auto& itr : value)
    {
        std::string station_name = itr.second.data();

        boost::shared_ptr<station> s = boost::make_shared<station>();
        s->ID(station_name);

        double easting = itr.second.get<double>("easting");
        s->x(easting);

        double northing = itr.second.get<double>("northing");
        s->y(northing);

        double elevation = itr.second.get<double>("elevation");
        s->z(elevation);

        std::string file = itr.second.get<std::string>("file");
        s->open(file);

        LOG_DEBUG << "New station created " << *s;
        _global->stations.push_back(s);

    }
}

void core::config_meshes(const pt::ptree& value)
{
    LOG_DEBUG << "Found meshes section";
#ifdef MATLAB
    _mesh = boost::make_shared<triangulation>(_engine);
#else
    _mesh = boost::make_shared<triangulation>();
#endif

    std::string dem = value.get<std::string>("DEM.file");
    LOG_DEBUG << "Found DEM mesh " << dem;

    _mesh->from_file(dem);
    if (_mesh->size_faces() == 0)
        BOOST_THROW_EXCEPTION(mesh_error() << errstr_info("Mesh size = 0!"));

    LOG_DEBUG << "Initializing DEM mesh attributes";
    #pragma omp parallel for
    for(size_t i = 0; i<_mesh->size_faces(); i++)
    {
        auto face = _mesh->face(i);
        face->slope();
        face->aspect();
        face->center();
        face->normal();
    }

}

void core::config_matlab(const pt::ptree& value)
{
    LOG_DEBUG << "Found matlab section";
#ifdef MATLAB
    //loop over the list of matlab options
    for (auto& jtr : value.get_obj())
    {
        const json_spirit::Pair& pair = jtr;
        const std::string& name = pair.name_;
        const json_spirit::Value& value = pair.value_;

        if (name == "mfile_paths")
        {
            LOG_DEBUG << "Found " << name;
            for (auto& ktr : value.get_obj()) //loop over all the paths
            {
                const json_spirit::Pair& pair = ktr;
                //                const std::string& name = pair.name_;
                const json_spirit::Value& value = pair.value_;


                _engine->add_dir_to_path(value.get_str());

            }
        }
    }
#endif
}

void core::config_output(const pt::ptree& value)
{
    LOG_DEBUG << "Found output section";
    //loop over the list of matlab options
    for (auto& itr : value)
    {
        output_info out;
        std::string outype = itr.first.data();
        if (outype == "timeseries"  )
        {

//            if (name == "timeseries")
//                out.type = output_info::timeseries;
//            out.face = _mesh->locate_face(out.easting, out.northing);
//            if (out.face == NULL)
//            {
//                LOG_WARNING << "Requested an output point that is not in the triangulation domain, skipping";
//            } else
//            {
//                _outputs.push_back(out);
//            }
            LOG_WARNING << "Timeseries output not implemented";
        }
        else if(outype == "mesh")
        {
            out.type = output_info::mesh;
            out.fname = itr.second.get<std::string>("base_name");

            for(auto& jtr: itr.second.get_child("format"))
            {
                LOG_DEBUG << "Output format found: " << jtr.second.data();
                if(jtr.second.data() == "vtu")
                    out.mesh_output_formats.push_back(output_info::mesh_outputs::vtu);
                if(jtr.second.data() == "vtp")
                    out.mesh_output_formats.push_back(output_info::mesh_outputs::vtp);
                if(jtr.second.data() == "ascii")
                    out.mesh_output_formats.push_back(output_info::mesh_outputs::ascii);
            }

        } else
        {
            LOG_WARNING << "Unknown output type: " << itr.second.data();
        }
        _outputs.push_back(out);
    }
}

void core::config_global(const pt::ptree& value)
{
    LOG_DEBUG << "Found global section";

    _global->_lat = value.get<double>("latitude");

    _global->_lon = value.get<double>("longitude");
    _global->_utc_offset = value.get<double>("UTC_offset");

}

std::pair<std::string,std::vector<std::pair<std::string,std::string>>> core::config_cmdl_options(int argc, char **argv)
{
    std::string version = "CHM version 0.1";

    std::string config_file = "CHM.config";

    po::options_description desc("Allowed options.");
    desc.add_options()
            ("help", "This message")
            ("version,v","Program version")
            ("config-file,f", po::value<std::string>(&config_file), "Configuration file to use. Can be passed without --config-file [-f] as well ")
            ("config,c",po::value<std::vector<std::string>>(),"Specifies a configuration parameter."
                    "This can over-ride existing values."
                    "The value is specified with a fully qualified config path. "
                    "For example:\n"
                    "-c config.Harder_precip_phase.const.b:1.5 -c config.debug.debug_level:\"error\"")
            ;


    //allow for specifgying config w/o --config
    po::positional_options_description p;
    p.add("config-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << std::endl;
        exit(1);
    }
    else if (vm.count("version"))
    {
        cout << version << std::endl;
        exit(1);
    }

    std::vector<std::pair<std::string,std::string>> config_extra;
    if(vm.count("config"))
    {
        boost::char_separator<char> sep(":");
        for(auto&itr : vm["config"].as< std::vector<std::string>>())
        {
            boost::tokenizer<boost::char_separator<char>> tok(itr,sep);
            std::vector<std::string> v;

            for(auto& jtr : tok)
                v.push_back(jtr);

            if(v.size() != 2)
                BOOST_THROW_EXCEPTION(io_error() << errstr_info("Config value of " + itr + " is invalid."));

            std::pair<std::string, std::string> override;
            override.first = v[0];
            override.second = v[1];
            config_extra.push_back(override);
        }
    }

    return std::make_pair(config_file,config_extra);
   // return config_extra;
}

void core::init(int argc, char **argv)
{
    BOOST_LOG_FUNCTION();

    //get any command line options
    auto cmdl_options = config_cmdl_options(argc, argv);

    pt::ptree cfg;
    try
    {
        //load the module config into it's own ptree
        pt::read_json(cmdl_options.first, cfg);

        LOG_DEBUG << "Reading configuration file " << argc;

        /*
         * The module config section is optional, but if it exists, we need it to
         * setup the modules, so check if it exists
         */
             //for each module: config pair
            for(auto& itr: cfg.get_child("config"))
            {
                pt::ptree module_config;

                //load the module config into it's own ptree
                pt::read_json(itr.second.data(), module_config);

                std::string module_name = itr.first.data();
                //replace the string config name with that config file
                cfg.put_child("config."+module_name,module_config);
 //               LOG_DEBUG << module_config.get<double>("const.b");
 //               LOG_DEBUG << cfg.get<double>("config.Harder_precip_phase.const.b");
            }

    }
    catch(pt::ptree_bad_path& e)
    {
        LOG_DEBUG << "Optional section Module config not found";
    }
    catch (pt::json_parser_error &e)
    {
        BOOST_THROW_EXCEPTION(config_error() << errstr_info( "Error reading file: " + e.filename() + " on line: " + std::to_string(e.line()) + "with error: " + e.message()));
    }



    //now apply any override or extra configuration parameters from the command line
    for(auto& itr:cmdl_options.second)
    {
        //check if we are overwriting something
        try
        {
            auto value = cfg.get<std::string>(itr.first);
            LOG_DEBUG << "Overwriting " << itr.first << "=" << value << " with " << itr.first << "=" << itr.second;

        }catch(pt::ptree_bad_path& e)
        {
            LOG_DEBUG << "Inserting new config " << itr.first << "=" << itr.second;
        }

        cfg.put(itr.first,itr.second);
    }



    /*
     * We expect the following sections:
     *  modules
     *  meshes
     *  forcing
     * The rest may be optional, and will override the defaults.
     */
    config_modules(cfg.get_child("modules"),cfg.get_child("config"));
    config_meshes(cfg.get_child("meshes"));
    config_forcing(cfg.get_child("forcing"));

    /*
     * We can expect the following sections to be optional.
     */
    try
    {
        config_debug(cfg.get_child("debug"));
    }catch(pt::ptree_bad_path& e)
    {
        LOG_DEBUG << "Optional section Debug not found";
    }

    try
    {
        config_output(cfg.get_child("output"));
    }catch(pt::ptree_bad_path& e)
    {
        LOG_DEBUG << "Optional section Output not found";
    }

    try
    {
        config_global(cfg.get_child("global"));
    }catch(pt::ptree_bad_path& e)
    {
        LOG_DEBUG << "Optional section Global not found";
    }

//#ifdef NOMATLAB
//            config_matlab(value);
//#endif
//        } else if (name == "output")
//        {
//            config_output(value);
//        } else if (name == "global")
//        {
//            config_global(value);
//        } else
//        {
//            const json_spirit::Pair& pair = itr;
//            const std::string& name = pair.name_;
//            LOG_INFO << "Unknown section '" << name << "', skipping";
//        }
//    }

    _cfg = cfg;

    LOG_DEBUG << cfg.get<int>("nproc");

    LOG_DEBUG << "Finished initialization";

    LOG_DEBUG << "Init variables mapping";
    _global->_variables.init_from_file("Un-init path");

    LOG_DEBUG << "Determining module dependencies";
    _determine_module_dep();

}

void core::_determine_module_dep()
{

    size_t size = _modules.size();

    //init a graph of the required size == num of modules
    Graph g(size);



    std::set<std::string> graphviz_vars;

    //loop through each module
    for (auto& module : _modules)
    {
        //Generate a  list of all variables,, provided from this module, and append to the total list, culling duplicates between modules.
        _provided_var_module.insert(module.first->provides()->begin(), module.first->provides()->end());

//        vertex v;
//        v.name = module.first->ID;
//        boost::add_vertex(v,g);
        g[module.first->IDnum].name=module.first->ID;
//        output_graph << module.first->IDnum << " [label=\"" << module.first->ID  << "\"];" << std::endl;

        //names.at(module.first->IDnum) =  module.first->ID;

        //check intermodule depends
        if (module.first->depends()->size() == 0)  //check if this module requires dependencies
        {
            LOG_DEBUG << "Module [" << module.first->ID << "], no inter-module dependenices";
        } else
        {
            LOG_DEBUG << "Module [" << module.first->ID << "] checking against...";
        }


        //make a copy of the depends for this module. we will then count references to each dependency
        //this will allow us to know what is missing
        std::map< std::string, size_t> curr_mod_depends;

        //populate a list of all the dependencies
        for (auto& itr : *(module.first->depends()))
        {
            curr_mod_depends[itr] = 0; //ref count init to 0
        }

        //iterate over all the modules,
        for (auto& itr : _modules)
        {
            //don't check against our module
            if (module.first->ID.compare(itr.first->ID) != 0)
            {
                //loop through each required variable of our current module
                for (auto& depend_var : *(module.first->depends()))
                {
                    //LOG_DEBUG << "\t\t[" << itr.first->ID << "] looking for var=" << depend_var;

                    auto i = std::find(itr.first->provides()->begin(), itr.first->provides()->end(), depend_var);
                    if (i != itr.first->provides()->end()) //itr provides the variable we are looking for
                    {
                        LOG_DEBUG << "\t\tAdding edge between " << module.first->ID << "[" << module.first->IDnum << "] -> " << itr.first->ID << "[" << itr.first->IDnum << "] for var=" << *i << std::endl;

                        //add the dependency from module -> itr, such that itr will come before module
                        edge e;
                        e.variable=*i;
//                        boost::add_edge(itr.first->IDnum, module.first->IDnum, g);

                        boost::add_edge(itr.first->IDnum, module.first->IDnum, e,g);

                        //output_graph << itr.first->IDnum << "->" << module.first->IDnum << " [label=\"" << *i << "\"];" << std::endl;
                        curr_mod_depends[*i]++; //ref count our variable

                        graphviz_vars.insert( *i);
                    }
                }
            }
        }


        bool missing_depends = false;

        std::stringstream ss;
        //build a list of ALL missing variables before dying
        for (auto& itr : curr_mod_depends)
        {
            if(itr.second == 0)
            {
                ss << "Missing inter-module dependencies for module [" << module.first->ID << "]: " << itr.first;
                missing_depends = true;
            }

        }
        if(missing_depends)
        {
            LOG_ERROR << ss.str();
            BOOST_THROW_EXCEPTION(module_error() << errstr_info( ss.str()));
        }

        //check if our module has any met file dependenices
        if (module.first->depends_from_met()->size() == 0)
        {
            LOG_DEBUG << "Module [" << module.first->ID << "], no met file dependenices";
        } else
        {
            LOG_DEBUG << "Module [" << module.first->ID << "] has met file dependencies";
        }


        LOG_DEBUG << "size " << _global->stations.size();
        //build a list of variables provided by the met files, culling duplicate variables from multiple stations.
        for (size_t i = 0; i < _global->stations.size(); i++)
        {
            auto vars = _global->stations.at(i)->list_variables();
            _provided_var_met_files.insert(vars.begin(), vars.end());
        }

        //check this modules met dependencies, bail if we are missing any.
        for(auto& depend_met_var : *(module.first->depends_from_met()))
        {
            auto i = std::find(_provided_var_met_files.begin(), _provided_var_met_files.end(), depend_met_var);
            if(i==_provided_var_met_files.end())
            {
                LOG_ERROR << "\t\t" <<depend_met_var<<"...[missing]";
                BOOST_THROW_EXCEPTION(module_error() << errstr_info ("Missing dependency for " + depend_met_var));
            }
            LOG_DEBUG << "\t\t" <<depend_met_var<<"...[ok]";
        }

    }

    //great filter file for gvpr
    std::ofstream gvpr("filter.gvpr");;

    std::string font = "Helvetica";
    int fontsize = 11;

    std::string edge_str = "E[edgetype == \"%s\"] {\n color=\"/paired12/%i\";\n fontsize=%i;\n     fontname=\"%s\"\n }";
    int idx=1;
    for(auto itr : graphviz_vars)
    {
        std::string edge = str_format(edge_str, itr.c_str(), idx, fontsize, font.c_str());
        idx++;
        gvpr << edge << std::endl;
    }

    gvpr.close();

    std::deque<int> topo_order;
    boost::topological_sort(g, std::front_inserter(topo_order));


    std::ostringstream ssdot;
    boost::write_graphviz(ssdot,g, boost::make_label_writer(boost::get(&vertex::name, g)),make_edge_writer(boost::get(&edge::variable, g)));
    std::string dot(ssdot.str());
    size_t pos = dot.find("\n");

    if (pos == std::string::npos)
        BOOST_THROW_EXCEPTION(config_error() << errstr_info("Unable to generate dot file"));

    //skip past the newline
    pos++;

//    Insert the following to make the chart go right to left, landscape
//    rankdir=LR;
//    {
//        node [shape=plaintext, fontsize=16];
//        "Module execution order"->"";
//    }
    dot.insert(pos, "rankdir=LR;\n{\n\tnode [shape=plaintext, fontsize=16];\n\t\"Module execution order\"->\"\";\n}\nsplines=polyline;\n");

    std::ofstream file;
    file.open("modules.dot.tmp");
    file << dot;
    file.close();

    //http://stackoverflow.com/questions/8195642/graphviz-defining-more-defaults

    std::system("gvpr -c -f filter.gvpr -o modules.dot modules.dot.tmp");
    std::system("dot -Tpdf modules.dot -o modules.pdf");
    std::remove("modules.dot.tmp");
    std::remove("filter.gvpr");



    std::stringstream ss;
    size_t order = 0;
    for(std::deque<int>::const_iterator i = topo_order.begin();  i != topo_order.end();  ++i)
    {
        ss << _modules.at(*i).first->ID << "->";
        _modules.at(*i).second = order;
        order++;
    }

    std::string     s = ss.str();
    LOG_DEBUG << "Build order: " << s.substr(0, s.length() - 2);


    //sort ascending based on make order number
    std::sort(_modules.begin(), _modules.end(),
            [](const std::pair<module, size_t>& a, const std::pair<module, size_t>& b)->bool
            {
                return a.second < b.second;
            });



    ss.str("");
    ss.clear();
    for (auto& itr : _modules)
    {
        ss << itr.first->ID << "->";
    }
    s = ss.str();
    LOG_DEBUG << "_modules order after sort: " << s.substr(0, s.length() - 2);

    // Parallel compilation ordering
//    std::vector<int> time(size, 0);
//    for (auto i = make_order.begin(); i != make_order.end(); ++i)
//    {
//        // Walk through the in_edges an calculate the maximum time.
//        if (boost::in_degree(*i, g) > 0)
//        {
//            Graph::in_edge_iterator j, j_end;
//            int maxdist = 0;
//            // Through the order from topological sort, we are sure that every
//            // time we are using here is already initialized.
//            for (boost::tie(j, j_end) = boost::in_edges(*i, g); j != j_end; ++j)
//                maxdist = (std::max)(time[boost::source(*j, g)], maxdist);
//            time[*i] = maxdist + 1;
//        }
//    }
//    boost::graph_traits<Graph>::vertex_iterator i, iend;
//    for (boost::tie(i, iend) = boost::vertices(g); i != iend; ++i)
//    {
//        LOG_DEBUG << "time_slot[" << time[*i] << "] = " << _modules.at(*i).first->ID << std::endl;
//    }



    //organize modules into sorted parallel data/domain chunks
    size_t chunks = 1; //will be 1 behind actual number as we are using this for an index
    size_t chunk_itr = 0;
    for (auto& itr : _modules)
    {
        LOG_DEBUG << "Chunking module: " << itr.first->ID;
        //first case, empty list
        if (_chunked_modules.size() == 0)
        {
            _chunked_modules.resize(chunks);
            _chunked_modules.at(0).push_back(itr.first);
        } else
        {
            if (_chunked_modules.at(chunk_itr).at(0)->parallel_type() == itr.first->parallel_type())
            {
                _chunked_modules.at(chunk_itr).push_back(itr.first);
            } else
            {
                chunk_itr++;
                chunks++;
                _chunked_modules.resize(chunks);
                _chunked_modules.at(chunk_itr).push_back(itr.first);
            }
        }
    }

    chunks = 0;
    for (auto& itr : _chunked_modules)
    {
        LOG_DEBUG << "Chunk " << (itr.at(0)->parallel_type() == module_base::parallel::data ? "data" : "domain") << " " << chunks << ": ";
        for (auto& jtr : itr)
        {
            LOG_DEBUG << jtr->ID;
        }
        chunks++;
    }

    LOG_DEBUG << "Initializing and allocating memory for timeseries";

    if (_global->stations.size() == 0)
        BOOST_THROW_EXCEPTION(forcing_error() << errstr_info("no stations"));

    #pragma omp parallel for
    for(size_t it = 0; it < _mesh->size_faces(); it++)
    {

        auto date = _global->stations.at(0)->date_timeseries();
        //auto size = _global->stations.at(0)->timeseries_length();
        Delaunay::Face_handle face = _mesh->face(it);

// face->init_time_series(_provided_var_module, date); /*length of all the vectors to initialize*/

        timeseries::date_vec d;
        d.push_back(date[0]);
        face->init_time_series(_provided_var_module, d);
    }


}

void core::run()
{
    timer c;
    LOG_DEBUG << "Running init() for each module";
    c.tic();
    for (auto& itr : _chunked_modules)
    {
        for (auto& jtr : itr)
        {
            jtr->init(_mesh);
        }

    }
    LOG_DEBUG << "Took " << c.toc<ms>() << "ms";

    LOG_DEBUG << "Starting model run";

    c.tic();

    size_t num_ts = 0;
    bool done = false;
    while (!done)
    {
        //ensure all the stations are at the same timestep
        boost::posix_time::ptime t;
        t = _global->stations.at(0)->now().get_posix(); //get first stations time
        for (size_t i = 1; //on purpose to skip first station
             i < _global->stations.size();
             i++)
        {
            if (t != _global->stations.at(i)->now().get_posix())
            {
                BOOST_THROW_EXCEPTION(forcing_timestep_mismatch()
                        << errstr_info("Timestep mismatch at station: " + _global->stations.at(i)->ID()));
            }
        }

        _global->_current_date = _global->stations.at(0)->now().get_posix();

        _global->update();

        LOG_DEBUG << "Timestep: " << _global->posix_time();

        size_t chunks = 0;
        for (auto& itr : _chunked_modules)
        {
            LOG_VERBOSE << "Working on chunk[" << chunks << "]:parallel=" << (itr.at(0)->parallel_type() == module_base::parallel::data ? "data" : "domain");
            c.tic();
            if (itr.at(0)->parallel_type() == module_base::parallel::data)
            {
                #pragma omp parallel for
                for (size_t i = 0; i < _mesh->size_faces(); i++)
                {
                    //module calls
                    for (auto& jtr : itr)
                    {
                        auto face = _mesh->face(i);
                        jtr->run(face, _global);
                    }
                }

            } else
            {
                //module calls for domain parallel
                for (auto& jtr : itr)
                {
                    jtr->run(_mesh, _global);
                }
            }
            LOG_DEBUG << "Took " << c.toc<ms>() << "ms";
            chunks++;

        }

        for (auto& itr : _outputs)
        {
            if (itr.type == output_info::output_type::mesh)
            {
               for(auto jtr : itr.mesh_output_formats)
               {
                   if(jtr == output_info::mesh_outputs::vtu)
                       _mesh->mesh_to_vtu(itr.fname+std::to_string(num_ts)+".vtu");
                   if(jtr == output_info::mesh_outputs::vtp)
                       _mesh->mesh_to_ascii(itr.fname+std::to_string(num_ts)+".vtp");
                   if(jtr == output_info::mesh_outputs::ascii)
                       LOG_WARNING << "Ascii output not implemented";
                       //_mesh->mesh_to_ascii(itr.fname+".vtp");
               }
            }
        }

        //save timestep to file
//        std::stringstream ss;
//        ss << "marmot" << num_ts << ".vtu";
//        _mesh->mesh_to_vtu(ss.str());
//
//
//        ss.str("");
//        ss.clear();
//        ss << "marmot" << num_ts << ".vtp";
//        _mesh->mesh_to_ascii(ss.str());

        num_ts++;

        c.tic();

//        #pragma omp parallel for
//        for (size_t i = 0; i < _mesh->size_faces(); i++)//update all the internal iterators
//        {
//            auto face = _mesh->face(i);
//            face->next();
//        }

        //update all the stations internal iterators to point to the next time step
        for (auto& itr : _global->stations)
        {
            if (!itr->next()) //
                done = true;
        }
       
            
        LOG_DEBUG << "Updating iterators took " << c.toc<ms>() <<"ms";

    }
    double elapsed = c.toc<s>();
    LOG_DEBUG << "Took " << elapsed << "s";



//    for (auto& itr : _outputs)
//    {
//#ifdef NOMATLAB
//        if (itr.plot)
//        {
//            //loop through all the request variables
//            for (auto& jtr : itr.variables)
//            {
//                if (itr.type == output_info::mesh)
//                {
//                    _mesh->plot(jtr);
//                } else if (itr.type == output_info::timeseries)
//                {
//                    _mesh->plot_time_series(itr.easting, itr.northing, jtr);
//                }
//            }
//        }
//#endif
//        if (itr.out_file != "")
//        {
//            _mesh->timeseries_to_file(itr.face, itr.out_file);
//        }
//    }

}
