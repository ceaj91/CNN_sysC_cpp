#ifndef HW_C
#define HW_C
#include "hw.hpp"

Hardware::Hardware(sc_module_name name):sc_module(name)																									
{
	SC_THREAD(proc);
	SC_THREAD(conv0);
	SC_THREAD(conv1);
	SC_THREAD(conv2);

	s_hw_t0.register_b_transport(this, &Hardware::b_transport0); //get data from DMA
	command_reg=0;

	toggle = SC_LOGIC_0;
	sig_conv0 = SC_LOGIC_0;
	sig_conv1 = SC_LOGIC_0;
	sig_conv2 = SC_LOGIC_0;

	conv1_counter = 0;
	conv2_counter = 0;

	// Maximum input size (second convolution input)
	input_image.reserve(CONV2_PADDED_PICTURE_SIZE * CONV2_PADDED_PICTURE_SIZE * CONV2_NUM_CHANNELS);
	// Maximum output size (first convolution output)
	output_image.reserve(32768);
	//std::cout << "HW constructed" << std::endl;
}

void Hardware::proc()
{

	int transfer_number = 1;
	int num_data_from_fifo=0;
	sc_time offset=SC_ZERO_TIME;
	hwdata_t fifo_read;
	#ifdef QUANTUM
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	#endif
	while(1)
	{
		while(	command_reg != 0b00001 && 
				command_reg != 0b00010 && 
				command_reg != 0b00011 && 
				command_reg != 0b00100 && 
				command_reg != 0b00101 && 
				command_reg != 0b00110 && 
				command_reg != 0b00111 && 
				command_reg != 0b01000 && 
				command_reg != 0b01001 && 
				command_reg != 0b01010 && 
				command_reg != 0b10000 )
		{
			#ifdef QUANTUM
		        qk.inc(sc_time(10, SC_NS));
		        offset = qk.get_local_time();
		        qk.set_and_sync(offset);
		        #else
		        offset += sc_time(10, SC_NS);
		        #endif
		}


		switch(command_reg)
		{
			case 0b00001: // load bias
				bias.clear();
				toggle = SC_LOGIC_1;
				p_out->write(toggle);
				for (int i = 0; i < BIAS_NUM_OF_PARAMETARS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}

					bias.push_back(fifo_read);
					
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle =  SC_LOGIC_1; 
				p_out->write(toggle);

			break;

			case 0b00010: // load CONV1(CONV0) weights
				conv1_counter = 0;
				conv2_counter = 0;
				weigts.clear();
				toggle = SC_LOGIC_1;
				p_out->write(toggle);
				for (int i = 0; i < CONV1_WEIGHTS_NUM; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}

					weigts.push_back(fifo_read);
					
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle =  SC_LOGIC_1; 
				p_out->write(toggle);

			break;

			case 0b00011: // load CONV0 input_picture
				input_image.clear();
				toggle = SC_LOGIC_1;
				p_out->write(toggle);
				
				for (int i = 0; i < CONV1_PADDED_PICTURE_SIZE * CONV1_PADDED_PICTURE_SIZE * CONV1_NUM_CHANNELS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}
					input_image.push_back(fifo_read);
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

			break;

			case 0b00100: // do CONV0

				sig_conv0 = SC_LOGIC_1;
				toggle = SC_LOGIC_0;
				p_out->write(toggle);

				while(sig_conv0 == SC_LOGIC_1)
				{
					#ifdef QUANTUM
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					#endif
				}

				// Conv0 is finished - signal to the CPU that is waiting
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

			break;
			
			case 0b00101: // load CONV1 input_picture
				input_image.clear();
				
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

				for (int i = 0; i < CONV2_PADDED_PICTURE_SIZE * CONV2_PADDED_PICTURE_SIZE * CONV2_NUM_CHANNELS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}
					input_image.push_back(fifo_read);
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

			break;

			case 0b00110: // load HALF of CONV2(CONV1) weights

				weigts.clear();
				toggle = SC_LOGIC_1;
				p_out->write(toggle);
				for (int i = 0; i < CONV2_HALF_WEIGHTS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}

					weigts.push_back(fifo_read);
					
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle =  SC_LOGIC_1; 
				p_out->write(toggle);

			break;

			case 0b00111: // do CONV1 with HALF WEIGHTS

				sig_conv1 = SC_LOGIC_1;

				toggle = SC_LOGIC_0;
				p_out->write(toggle);

				while(sig_conv1 == SC_LOGIC_1)
				{
					#ifdef QUANTUM
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					#endif
				}

				toggle = SC_LOGIC_1;
				p_out->write(toggle);

			break;

			case 0b01000: // send output image to Memory component

				toggle = SC_LOGIC_0;
				p_out->write(toggle);

				for (unsigned int j = 0; j < output_image.size(); ++j)
				{
					#ifdef QUANTUM
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					#else
					offset += sc_time(10, SC_NS);
					#endif

					while(!p_fifo_out->nb_write(output_image[j]))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}
					toggle = SC_LOGIC_0;
					p_out->write(toggle);
				}
				// WAIT 10ns MORE TO DMA GET LAST NUMBER, AFTER THAT SIGNAL TO PROCESSOR THAT TRANSFER IS FINISHED
				#ifdef QUANTUM
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				#else
				offset += sc_time(10, SC_NS);
				#endif

				output_image.clear();

				toggle = SC_LOGIC_1;
				p_out->write(toggle);
			break;

			case 0b01001: // load CONV2 input_picture
				input_image.clear();
				
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

				for (int i = 0; i < CONV3_PADDED_PICTURE_SIZE * CONV3_PADDED_PICTURE_SIZE * CONV3_NUM_CHANNELS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}
					input_image.push_back(fifo_read);
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				toggle = SC_LOGIC_1;
				p_out->write(toggle);

			break;

			case 0b01010: // load 1/4 of CONV2 weights
				weigts.clear();
				toggle = SC_LOGIC_1;
				p_out->write(toggle);
				for (int i = 0; i < CONV3_SPLIT_WEIGHTS; ++i)
				{
					while(!p_fifo_in->nb_read(fifo_read))
					{
						#ifdef QUANTUM
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#else
						offset += sc_time(10, SC_NS);
						#endif
					}

					weigts.push_back(fifo_read);
					
					toggle =  SC_LOGIC_0; 
					p_out->write(toggle);
				}
				
				toggle =  SC_LOGIC_1; 
				p_out->write(toggle);

			break;

			case 0b10000: // do CONV2 with 1/4 of weights

				sig_conv2 = SC_LOGIC_1;

				toggle = SC_LOGIC_0;
				p_out->write(toggle);

				while(sig_conv2 == SC_LOGIC_1)
				{
					#ifdef QUANTUM
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					#endif
				}

				toggle = SC_LOGIC_1;
				p_out->write(toggle);
			break;
			
			default:
				cout<<" HW: Nothing to be done."<<endl;
			break;
		}
		
		command_reg = 0b00000;
	}

}

void Hardware::b_transport0(pl_t& pl, sc_time& offset)
{
	tlm_command cmd    = pl.get_command();
	uint64 adr         = pl.get_address();
	const unsigned char *buf = pl.get_data_ptr();
	unsigned int len   = pl.get_data_length();
	switch(cmd)
	{
		case TLM_WRITE_COMMAND:
			command_reg = int(*buf);
			pl.set_response_status(TLM_OK_RESPONSE);
			break;
		case TLM_READ_COMMAND:
			break;
		default:
			pl.set_response_status( TLM_COMMAND_ERROR_RESPONSE );
	}
	offset += sc_time(10, SC_NS);

}

void Hardware::conv0()
{

	sc_time offset=SC_ZERO_TIME;
	#ifdef QUANTUM
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	#endif
	
	while(1)
	{
		while(sig_conv0 == SC_LOGIC_0)
		{
			#ifdef QUANTUM
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			#endif
			
		}

		#ifdef PRINTS
		cout << "Conv0 start at: " << sc_time_stamp() << endl;
		#endif

		int start_address_weights = 0; 
		int start_address_bias = 0;
		int filter_size = 3;
		int num_of_channels = 3;
		int num_of_filters = 32;
		int img_size = 34;

		output_image.clear();
		for (int filter = 0; filter < num_of_filters; ++filter)
		{
			for (int i = 0; i < img_size-2; i++)
			{
				for (int j = 0; j < img_size-2; j++)
				{
					deque<hwdata_t> image_slice;
					image_slice.clear();
					for (int channel_slice = 0; channel_slice < num_of_channels; ++channel_slice)
					{	
						for (int i_slice = i; i_slice < i+filter_size; ++i_slice)
						{
							for (int j_slice = j; j_slice < j+filter_size; ++j_slice)
							{
								image_slice.push_back(input_image[channel_slice * (img_size * img_size) + i_slice*img_size + j_slice]);	

								#ifdef IMG_SLICE_TO_BUFFER_DELAY
									#ifdef QUANTUM
									qk.inc(sc_time(10, SC_NS));
									offset = qk.get_local_time();
									qk.set_and_sync(offset);
									#endif
								#endif
								
							}
						}
					}
				
					hwdata_t conv_sum = 0;
					for (int channel = 0; channel < num_of_channels; ++channel)
						{

							if((channel+1) % CONV0_PARALELISM == 0)
							{
								#ifdef QUANTUM
								qk.inc(sc_time(10, SC_NS));
								offset = qk.get_local_time();
								qk.set_and_sync(offset);
								#endif
							}

							for (int i_slice = 0; i_slice < filter_size; ++i_slice)
							{
								for (int j_slice = 0; j_slice < filter_size; ++j_slice)
								{
									
									conv_sum = conv_sum + image_slice[channel * (filter_size * filter_size) + i_slice*filter_size + j_slice] * 
									weigts[start_address_weights + filter*(filter_size*filter_size*num_of_channels)  + channel * (filter_size * filter_size) + i_slice*filter_size + j_slice];
									
								}
							}
						}
					if(conv_sum + bias[start_address_bias + filter] > 0)
						output_image.push_back(conv_sum + bias[start_address_bias + filter]);
					else	
						output_image.push_back(0);
					
					#ifdef COMPARE_AND_BIAS_ADDITION_DELAY
						#ifdef QUANTUM
						qk.inc(sc_time(20, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#endif
					#endif
				}
			}
		}
		qk.set_and_sync(offset);
		#ifdef PRINTS
		cout << "Conv0 finished at: " << sc_time_stamp() << endl;
		#endif

		sig_conv0 = SC_LOGIC_0;
	}
}

void Hardware::conv1()
{

	sc_time offset=SC_ZERO_TIME;
	#ifdef QUANTUM
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	#endif

	while(1)
	{
		while(sig_conv1 == SC_LOGIC_0)
		{
			#ifdef QUANTUM
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			#endif
		}
		
		#ifdef PRINTS
		cout << "Conv1 - part" << conv1_counter+1 << " start at: " << sc_time_stamp() << endl;
		#endif

		int start_address_weights = 0;
		int start_address_bias = CONV1_NUM_BIAS;
		int filter_size = 3;
		int num_of_channels = 32;
		int num_of_filters = 16;
		int img_size = 18;

		for (int filter = 0; filter < num_of_filters; ++filter)
		{
			for (int i = 0; i < img_size-2; i++)
			{
				for (int j = 0; j < img_size-2; j++)
				{
					deque<hwdata_t> image_slice;
					image_slice.clear();
					for (int channel_slice = 0; channel_slice < num_of_channels; ++channel_slice)
					{	
						for (int i_slice = i; i_slice < i+filter_size; ++i_slice)
						{
							for (int j_slice = j; j_slice < j+filter_size; ++j_slice)
							{
								image_slice.push_back(input_image[channel_slice * (img_size * img_size) + i_slice*img_size + j_slice]);	

								#ifdef IMG_SLICE_TO_BUFFER_DELAY
									#ifdef QUANTUM
									qk.inc(sc_time(10, SC_NS));
									offset = qk.get_local_time();
									qk.set_and_sync(offset);
									#else
									offset += sc_time(10, SC_NS);
									#endif	
								#endif		
							}
						}
					}
		
					hwdata_t conv_sum = 0;
					for (int channel = 0; channel < num_of_channels; ++channel)
					{
						if((channel+1) % CONV1_PARALELISM == 0)
						{
							#ifdef QUANTUM
							qk.inc(sc_time(10, SC_NS));
							offset = qk.get_local_time();
							qk.set_and_sync(offset);
							#else
							offset += sc_time(10, SC_NS);
							#endif
						}

						for (int i_slice = 0; i_slice < filter_size; ++i_slice)
						{
							for (int j_slice = 0; j_slice < filter_size; ++j_slice)
							{
								
								conv_sum = conv_sum + image_slice[channel * (filter_size * filter_size) + i_slice*filter_size + j_slice] * 
								weigts[start_address_weights + filter*(filter_size*filter_size*num_of_channels)  + channel * (filter_size * filter_size) + i_slice*filter_size + j_slice];

							}
						}
					}
					if(conv_sum + bias[16*conv1_counter + start_address_bias + filter] > 0)
						output_image.push_back(conv_sum + bias[16*conv1_counter + start_address_bias + filter]);
					else	
						output_image.push_back(0);
		
					#ifdef COMPARE_AND_BIAS_ADDITION_DELAY
						#ifdef QUANTUM
						qk.inc(sc_time(20, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#endif
					#endif
					
				}
			}
		}

		qk.set_and_sync(offset);
		#ifdef PRINTS
		cout << "Conv1 - part" << conv1_counter+1 << " finished at: " << sc_time_stamp() << endl;
		#endif

		conv1_counter++;
		sig_conv1 = SC_LOGIC_0;
	}
}


void Hardware::conv2()
{

	sc_time offset=SC_ZERO_TIME;
	#ifdef QUANTUM
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	#endif
	
	while(1)
	{
		while(sig_conv2 == SC_LOGIC_0)
		{
			#ifdef QUANTUM
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			#endif
		}
		
		#ifdef PRINTS
		cout << "Conv2 - part" << conv2_counter+1 << " start at: " << sc_time_stamp() << endl;
		#endif
		
		int img_size = 10;
		int start_address_weights = 0;
		int start_address_bias = CONV1_NUM_BIAS + CONV2_NUM_BIAS;
		int filter_size = 3;
		int num_of_channels = 32;
		int num_of_filters = 64 / 4;

		for (int filter = 0; filter < num_of_filters; ++filter)
		{
			for (int i = 0; i < img_size-2; i++)
			{
				for (int j = 0; j < img_size-2; j++)
				{
					deque<hwdata_t> image_slice;
					image_slice.clear();
					for (int channel_slice = 0; channel_slice < num_of_channels; ++channel_slice)
					{	
						for (int i_slice = i; i_slice < i+filter_size; ++i_slice)
						{
							for (int j_slice = j; j_slice < j+filter_size; ++j_slice)
							{
								image_slice.push_back(input_image[channel_slice * (img_size * img_size) + i_slice*img_size + j_slice]);	
							
								#ifdef IMG_SLICE_TO_BUFFER_DELAY
									#ifdef QUANTUM
									qk.inc(sc_time(10, SC_NS));
									offset = qk.get_local_time();
									qk.set_and_sync(offset);
									#else
									offset += sc_time(10, SC_NS);
									#endif	
								#endif	
								
							}
						}
					}
	
					hwdata_t conv_sum = 0;
					for (int channel = 0; channel < num_of_channels; ++channel)
						{

							if((channel+1) % CONV2_PARALELISM == 0)
							{
								#ifdef QUANTUM
								qk.inc(sc_time(10, SC_NS));
								offset = qk.get_local_time();
								qk.set_and_sync(offset);
								#else
								offset += sc_time(10, SC_NS);
								#endif
							}

							for (int i_slice = 0; i_slice < filter_size; ++i_slice)
							{
								for (int j_slice = 0; j_slice < filter_size; ++j_slice)
								{
									
									conv_sum = conv_sum + image_slice[channel * (filter_size * filter_size) + i_slice*filter_size + j_slice] * 
									weigts[start_address_weights + filter*(filter_size*filter_size*num_of_channels)  + channel * (filter_size * filter_size) + i_slice*filter_size + j_slice];

								}
							}
						}
					if(conv_sum + bias[16*conv2_counter + start_address_bias + filter] > 0)
						output_image.push_back(conv_sum + bias[16*conv2_counter + start_address_bias + filter]);
					else	
						output_image.push_back(0);

					
					#ifdef COMPARE_AND_BIAS_ADDITION_DELAY
						#ifdef QUANTUM
						qk.inc(sc_time(20, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						#endif
					#endif
				}
			}
		}

		qk.set_and_sync(offset);
		#ifdef PRINTS
		cout << "Conv2 - part" << conv2_counter+1 << " finished at: " << sc_time_stamp() << endl;
		#endif
		
		conv2_counter++;
		sig_conv2 = SC_LOGIC_0;
	}
}


#endif