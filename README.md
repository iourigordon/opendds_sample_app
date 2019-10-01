# opendds_sample_app
OpenDDS sample app

1. OpenDDS Installation
  Download and untar OpenDDS-3.13.2
  cd OpenDDS-3.13.2
  ./configure --std=c++14 --prefix /opt/opendds
  make
  make instalpp l
  
  After these steps OpenDDS will be installed under /open/opendds. If you are compiling application against
  installed OpenDDS set DDS_ROOT to /opt/opendds/share/dds, if you want to compile against local build set DDS_ROOT to a 
  directory that has OpenDDS sources
  
2. Compiling opendds_sample_app

  Run make twice, there is an issue with generated OpenDDS files, makefile does not detect them properly
  There are two clean targets:
    make clean (will clean all the objects, but will not remove generated files)
    make clean_all (will remove objects and generated files. After this you'll need to run make twice)
    
3. Running  opendds_sample_app

  opendds_sample_app is very simple application that shows to configure OpenDDS to run in RTPS mode in two ways. It demonstrates
  that configered OpenDDS will work in a single thread mode and in a mode where publisher and subscriber are split into different
  threads.
  
  Running opendds_sample_app using rtps.ini file, as described in OpenDDS DevGuide.
  
  ./sample_app -DCPSConfigFile rtps.ini single (will run opendds_sample_app in a single thread mode, SUBSCRIPTION_MATCH condition
                                                is ignored)
                                                
  ./sample_app -DCPSConfigFile rtps.ini multi (will run opendds_sample_app in two threads mode, SUBSCRIPTION_MATCH condition
                                                is used )
                                                
  Running opendds_sample_app without rtps.ini, so RTPS transport is setup programmaticaly
  
  ./sample_app single (will run opendds_sample_app in a single thread mode, SUBSCRIPTION_MATCH condition is ignored)
  
  ./sample_app multi  (will run opendds_sample_app in two threads mode, SUBSCRIPTION_MATCH condition is used )
