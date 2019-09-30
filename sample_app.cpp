#include <pthread.h>
#include <iostream>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include "dds/DCPS/StaticIncludes.h"
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst_rch.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>

#include "MessengerTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"

using namespace std;
using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

DDS::DomainParticipant_var m_participant;

int run_subscriber(bool block, DDS::Topic_ptr topic)
{
    DDS::Subscriber_var subscriber = m_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
																	  DDS::SubscriberListener::_nil(),
																	  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(subscriber.in())) {
		fprintf(stdout,"failed to create subscriber\n");
    }

    // Create DataReader
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);

    DDS::DataReader_var reader = subscriber->create_datareader(topic,
															   DATAREADER_QOS_DEFAULT,
															   listener.in(),
															   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(reader.in())) {
		fprintf(stdout,"failed to create data reader\n");
    }

    Messenger::MessageDataReader_var reader_i = Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil(reader_i.in())) {
		fprintf(stdout,"failed to narrow data reader\n");
    }

    if (block) {
        // Block until Publisher completes
        DDS::StatusCondition_var condition = reader->get_statuscondition();
        condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        DDS::ConditionSeq conditions;
        DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };
        DDS::Duration_t timeout = { 30, 0 }; // 30 seconds

        do {
            if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" get_subscription_matched_status() failed!\n")), -1);
            }

            if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -") 
                                    ACE_TEXT(" wait failed!\n")), -1);
            }

        } while (matches.current_count > 0);

        ws->detach_condition(condition);
    }
    return 0;
}

int run_publisher(DDS::Topic_ptr topic) {
	DDS::Publisher_var publisher = m_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
																   DDS::PublisherListener::_nil(),
																   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher.in())) {
		fprintf(stdout,"Failed to create publisher\n");
    }

    // Create DataWriter
    DDS::DataWriter_var writer = publisher->create_datawriter(topic,
															  DATAWRITER_QOS_DEFAULT,
															  DDS::DataWriterListener::_nil(),
															  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in())) {
		fprintf(stdout,"Failed to create data writer\n");
    }

    Messenger::MessageDataWriter_var message_writer = Messenger::MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
		fprintf(stdout,"Failed to narrow data write\n");
    }


    DDS::Duration_t timeout = { 30, 0 };
    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };

    do {
		if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
			fprintf(stdout,"timed out waiting for subscriber\n");
		}

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
			fprintf(stdout,"got a match\n");
      }

    } while (matches.current_count < 1);

    ws->detach_condition(condition);

    // Write samples
    Messenger::Message message;
    message.subject_id = 99;

    message.from       = CORBA::string_dup("Comic Book Guy");
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 0;

    for (int i = 0; i < 10; i++) {
		DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);

		if (error != DDS::RETCODE_OK) {
			fprintf(stdout,"Failed to write a message\n"); 
		}
	}

    // Wait for samples to be acknowledged
    if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
		fprintf(stdout,"Failed to receive acknowledgement\n");    
    }
    return 0;
}


DDS::Topic_ptr create_msg_topic() {
    DDS::Topic_ptr topic; 
	Messenger::MessageTypeSupport_var ts = new Messenger::MessageTypeSupportImpl;

	if (ts->register_type(m_participant, "") != DDS::RETCODE_OK) {
		fprintf(stdout,"failed to register type\n");
	}

	CORBA::String_var type_name = ts->get_type_name();
	fprintf(stdout,"type_name = %s\n", type_name.in());

	topic = m_participant->create_topic("Movie Discussion List",
		    							  type_name,
			    						  TOPIC_QOS_DEFAULT,
				    					  0,
					    				  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic)) {
	    fprintf(stdout,"Failed to create topic\n");
        return NULL;
    }
    return topic;
}

void run_test(DDS::Topic_ptr topic) 
{
    run_subscriber(false,topic);
    run_publisher(topic);
}

void* subscriber_thread(void* args)
{
    run_subscriber(true,create_msg_topic());
}

void* publisher_thread(void* args)
{
    run_publisher(create_msg_topic());
}

#if 0
int main(int argc, char* argv[]) {

    OpenDDS::DCPS::set_DCPS_debug_level(10);
    OpenDDS::DCPS::Transport_debug_level = 6; 

	TheServiceParticipant->set_default_discovery(::OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
	const std::string config_name = "PerfTestConfig";
	OpenDDS::DCPS::TransportConfig_rch config = TheTransportRegistry->create_config(config_name);
	OpenDDS::DCPS::TransportInst_rch trans_inst = TheTransportRegistry->create_inst(config_name, "rtps_udp");
	OpenDDS::DCPS::RtpsUdpInst_rch udp_rtps_inst = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::RtpsUdpInst>(trans_inst);

	//udp_rtps_inst->use_multicast_ = true;
	//udp_rtps_inst->local_address("0.0.0.0:5001");
	udp_rtps_inst->datalink_release_delay_ = 10000;
	//udp_rtps_inst->multicast_interface_ = "wlp3s0";
#ifdef OPEN_DDS_SOURCE
    public native String getLocalAddress();
    public native void setLocalAddress(String la);

    public native boolean isUseMulticast();
    public native void setUseMulticast(boolean um);

    public native String getMulticastGroupAddress();
    public native void setMulticastGroupAddress(String mga);
#endif

    //fprintf(stdout,"transport_type = %s\n",trans_inst->transport_type);

    //udp_rtps_inst->dump();
	config->instances_.push_back(trans_inst);
	TheTransportRegistry->global_config(config);

	//ACE::init();
	DDS::DomainParticipantFactory_var participant_factory = TheServiceParticipant->TheParticipantFactory;

	::DDS::DomainParticipantFactoryQos factory_qos;
	participant_factory->get_qos (factory_qos);
	factory_qos.entity_factory.autoenable_created_entities=false;
	participant_factory->set_qos(factory_qos);

	//OpenDDS domain participant qos is realy weird, just octest sequences
	//for now let's create domain participant with default qos
	m_participant = participant_factory->create_participant(0,
															PARTICIPANT_QOS_DEFAULT,
															0,
															OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    
    trans_inst->dump();
	Messenger::MessageTypeSupport_var ts = new Messenger::MessageTypeSupportImpl;

	if (ts->register_type(m_participant, "") != DDS::RETCODE_OK) {
		fprintf(stdout,"failed to register type\n");
	}

	CORBA::String_var type_name = ts->get_type_name();
	fprintf(stdout,"type_name = %s\n", type_name.in());
	m_topic = m_participant->create_topic("Movie Discussion List",
										  type_name,
										  TOPIC_QOS_DEFAULT,
										  0,
										  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(m_topic.in())) {
		fprintf(stdout,"Failed to create topic\n");
    }

	run_test();

  // Clean-up!
    m_participant->delete_contained_entities();
    participant_factory->delete_participant(m_participant.in());

    TheServiceParticipant->shutdown();

	return 0;
}
#else

int main(int argc, char* argv[])
{
    bool threads;
    pthread_t sub_id, pub_id;
    DDS::DomainId_t domain;
    DDS::DomainParticipantFactory_var dpf;

    fprintf(stdout,"argc = %d\n",argc);
    if ((argc != 2) && (argc != 4))
    {
        fprintf(stdout,"Need to supply either '-DSCPConfigFile filename' or 'multi/single' for number of threads to create\n");
        return 0;
    }

    threads = false;
    switch(argc) {
        case 2: {
            dpf = TheParticipantFactory;

            TransportConfig_rch config = TransportRegistry::instance()->create_config("ApexAiConfig");
            TransportInst_rch inst = TransportRegistry::instance()->create_inst("rtps_tran","rtps_udp");
 
            RtpsUdpInst_rch rui = static_rchandle_cast<RtpsUdpInst>(inst);
            rui->handshake_timeout_ = 1;

            config->instances_.push_back(inst);
            TransportRegistry::instance()->global_config(config);

            domain = 0;
            bool multicast = true;
            unsigned int resend = 1;
            std::string partition, governance, permissions;
            int defaultSize = 0;

            RtpsDiscovery_rch disc = make_rch<RtpsDiscovery>("RtpsDiscovery");
            rui->use_multicast_ = true;

            TheServiceParticipant->add_discovery(static_rchandle_cast<Discovery>(disc));
            TheServiceParticipant->set_repo_domain(domain, disc->key());
            DDS::DomainParticipantQos dp_qos;
            dpf->get_default_participant_qos(dp_qos);

            rui->dump();
            if (strcmp(argv[1],"multi") == 0) {
                threads = true;
            }
            break;
        }
        case 4:
            domain = 42;
            dpf  = TheParticipantFactoryWithArgs(argc,argv);
            if (strcmp(argv[1],"multi") == 0) {
                threads = true;
            }
            break;
        default:
            fprintf(stdout,"only one or two arguments should be supplied\n");
            return 0;
            break;
    }

    m_participant = dpf->create_participant(domain,PARTICIPANT_QOS_DEFAULT,0,DEFAULT_STATUS_MASK);

    if (!m_participant) {
      std::cerr << "Could not connect to domain " << std::endl;
      TheServiceParticipant->shutdown();
      return EXIT_FAILURE;
    }

    if (!threads) {
        fprintf(stdout,"Running single thread test\n");
	    run_test(create_msg_topic());
    } else {
        fprintf(stdout,"Spinning threads\n");
        pthread_create(&sub_id,NULL,subscriber_thread,NULL);
        pthread_create(&pub_id,NULL,publisher_thread,NULL);
        
        fprintf(stdout,"Joining threads\n");
        pthread_join(sub_id,NULL);
        pthread_join(pub_id,NULL);
    }

    m_participant->delete_contained_entities();
    dpf->delete_participant(m_participant);
    TheServiceParticipant->shutdown();

    return 0;
}
#endif
