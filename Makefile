#!/usr/bin/bash

export DDS_ROOT=/opt/opendds/share/dds

GEN_DIR:=gen
OBJ_DIR:=obj

DDS_INSTALL:=/opt/opendds
TAO_IDL:=$(DDS_ROOT)/../..//bin/tao_idl
OPENDDS_IDL:=$(DDS_ROOT)/../../bin/opendds_idl
TAO_INCLUDES:=-I$(DDS_ROOT)../../include/tao -I$(DDS_ROOT)/../../include

LDFLAGS:=-L/opt/opendds/lib -lpthread
#LDLIBS:=-lOpenDDS_Rtps -lOpenDDS_Rtps_Udp -lOpenDDS_InfoRepoDiscovery -lOpenDDS_Dcps -lTAO_BiDirGIOP -lTAO_PI -lTAO_CodecFactory -lTAO_PortableServer -lTAO_AnyTypeCode -lTAO -lACE

LDLIBS:=-lOpenDDS_Rtps_Udp -lOpenDDS_Rtps -lOpenDDS_Multicast -lOpenDDS_Udp -lOpenDDS_Tcp -lOpenDDS_InfoRepoDiscovery -lOpenDDS_Dcps -lTAO_BiDirGIOP -lTAO_PI -lTAO_CodecFactory -lTAO_PortableServer -lTAO_AnyTypeCode -lTAO -lACE

CXXFLAGS= -g -std=c++14 -I/opt/opendds/include -I./gen/ -pthread

SRC_CPP:=sample_app.cpp DataReaderListenerImpl.cpp

GEN_CPP:=MessengerC.cpp MessengerS.cpp MessengerTypeSupportC.cpp MessengerTypeSupportS.cpp MessengerTypeSupportImpl.cpp

GEN_H:= MessengerC.h MessengerS.h MessengerTypeSupportC.h MessengerTypeSupportS.h MessengerTypeSupportImpl.h

OBJ_GEN_FILES:=$(addprefix $(OBJ_DIR)/,$(GEN_CPP:%.cpp=%.o))
OBJ_SRC_FILES:=$(addprefix $(OBJ_DIR)/,$(SRC_CPP:%.cpp=%.o))

sample_app: idl_gen $(OBJ_SRC_FILES) $(OBJ_GEN_FILES) 
	$(CXX) $(CXXFLAGS) $(OBJ_SRC_FILES) $(OBJ_GEN_FILES) -o sample_app $(LDFLAGS) $(LDLIBS)

.PHONY: idl_gen
idl_gen: create_dirs
	@echo "Invoking tao_idl"
	@$(TAO_IDL) -Wb,pre_include=ace/pre.h -Wb,post_include=ace/post.h -I/home/iouri/Downloads/OpenDDS-3.13.2/ACE_wrappers/TAO -Sa -St -I/home/iouri/Download/OpenDDS-3.13.2 Messenger.idl -o $(GEN_DIR)
	@echo "Invoking opendds_idl"
	@$(OPENDDS_IDL) -Sa -St Messenger.idl -o $(GEN_DIR)
	@echo "Generating TypeSupport"
	@$(TAO_IDL) -Wb,pre_include=ace/pre.h -Wb,post_include=ace/post.h -I/home/iouri/Downloads/OpenDDS-3.13.2/ACE_wrappers/TAO -Sa -St -I/home/iouri/Download/OpenDDS-3.13.2 $(GEN_DIR)/MessengerTypeSupport.idl  -o $(GEN_DIR) $(TAO_INCLUDES)

$(OBJ_DIR)/%.o: ./%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(GEN_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

create_dirs:
	@mkdir -p $(GEN_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -rf $(OBJ_DIR)
	@rm -f sample_app

clean_all: clean
	@rm -rf $(GEN_DIR)
