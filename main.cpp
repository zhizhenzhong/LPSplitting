#include <iostream>
#include <string>
#include "LPSplitting.h"
#include <time.h>
#include <math.h>

Network network;
int total_traffic_no;

//all lightpatsh / number of successful traffic
int lightpath_control = 0;
int overall_traffic_success = 0;

//Baseline lightpatha / number of successful traffic
int baseline_lightpath_control;
int baseline_overall_traffic_success;

//Incremental lightpaths before splitting / number of successful traffic
int pre_split_lightpath_control = 0;
int pre_split_overall_traffic_success = 0;

//lightpaths after splitting
int split_lightpath_control = 0;

int combine_control = 0;

Node::Node()
{
	occupy_rx_oe = 0;
	occupy_tx_eo = 0;
}

Optical::Optical()
{
	optical_src = 0;
	optical_dst = 0;
	optical_length = 0;
	optical_link_capacity = 0;
	spectrum_occupy = NULL;
}

Lightpath::Lightpath()
{
	lightpath_src = 0;
	lightpath_dst = 0;
	lightpath_modulation = 0;
	lightpath_residual_bw = 0;
	lightpath_supporting_bw = 0;
	lightpath_used_slots_num = 0;
	lightpath_used_slots.clear();
	lightpath_path.clear();
}

Traffic::Traffic()
{
	traffic_ID = 0;
	traffic_src= 0;
	traffic_dst = 0;
	traffic_bw = 0;
	traffic_actual_bw = 0;
	traffic_success = 0;
	traffic_hops = 0;
}

Network::Network()
{
	baseline_traffic = NULL;
	incremental_traffic = NULL;
	optical_database = NULL;
	lightpath_number = 0;
}

void Network::Read_Topo()
{
	bool findData=0;
	FILE *read_topo_file;

	if((read_topo_file = fopen("splitting_topo_NSFNET.dat", "r")) == 0)
	{
		cout<<"splitting_topo_NSFNET.dat NOT OPEN!!"<<endl;
		return;
	}

	optical_database = new Optical[TOPO_LINK_NUM];
	Optical *i = optical_database;
	while (!feof(read_topo_file))  //feof detect whether file end is reached
	{
		fscanf(read_topo_file, "%d %d %d\n", &(i->optical_src), &(i->optical_dst), &(i->optical_length));		
		i++;
	}
	fclose(read_topo_file);

	for (int i=0; i<TOPO_LINK_NUM; i++)
	{
		optical_database[i].spectrum_occupy = new int[SPECTRUM_SLOT_NUM];
		for (int j = 0; j < SPECTRUM_SLOT_NUM; j++)
		{
			optical_database[i].spectrum_occupy[j] = 0;
		}
	}
}

void Network::Generate_Traffic(float mu)
{
	//generate baseline traffic
	srand( (unsigned)time(NULL) ); //seed for random
	baseline_traffic = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];

	//range: 1~80, where 1 represent 2.5Gbps, 2.5~200Gbps, 
	//total amount is 80*(24*23)/2 = 22080, which is 55200 Gbps
	int temp_sum = 0;
	int x = 0;

	for (int r = 0; r < TOPO_NODE_NUM; r++)
	{
		for (int s = 0; s < TOPO_NODE_NUM; s++)
		{
			if (s != r)
			{
				baseline_traffic[x].traffic_src = r;
				baseline_traffic[x].traffic_dst = s;
				x++;
			}
		}
	}

	vector<int> req;
	for (int d = 0; d < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); d++)
	{
		req.push_back(d);
	}

	while (req.size() > 0)
	{
		int rand1 = rand() % req.size();
		baseline_traffic[req[rand1]].traffic_bw = rand() % (BANDWIDTH_RANGE-1) + 2; //2 ~ 20
		float temp_bw = baseline_traffic[req[rand1]].traffic_bw;
		vector<int>::iterator it;
		int del1 = req[rand1];
		for(it=req.begin();it!=req.end();)
		{
			if(*it == del1)
				it=req.erase(it); 
			else
				++it;
		}

		int rand2 = rand() % req.size();
		baseline_traffic[req[rand2]].traffic_bw = BANDWIDTH_RANGE - temp_bw;
		vector<int>::iterator it2;
		int del2 = req[rand2];
		for(it2=req.begin();it2!=req.end();)
		{
			if(*it2 == del2)
				it2=req.erase(it2); 
			else
				++it2;
		}
	}

	bubble_sort(baseline_traffic, TOPO_NODE_NUM*(TOPO_NODE_NUM-1));

	float accummulate = 0;
	float accumulate_incremental = 0;
	incremental_traffic = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];
	
	for (int y = 0; y < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); y++)
	{
		incremental_traffic[y].traffic_ID = baseline_traffic[y].traffic_ID;
		incremental_traffic[y].traffic_src = baseline_traffic[y].traffic_src;
		incremental_traffic[y].traffic_dst = baseline_traffic[y].traffic_dst;
		incremental_traffic[y].traffic_bw = mu*baseline_traffic[y].traffic_bw;
		incremental_traffic[y].traffic_actual_bw = baseline_traffic[y].traffic_actual_bw;
		incremental_traffic[y].traffic_success = baseline_traffic[y].traffic_success;

		accummulate += baseline_traffic[y].traffic_bw;
		accumulate_incremental += incremental_traffic[y].traffic_bw;
	}

	cout<<"-------------------"<<endl;
	cout<<"Total baseline bandwidth: "<<accummulate<<endl;
	cout<<"Total incremental(mu="<<mu<<") bandwidth: "<<accumulate_incremental<<endl;
	cout<<"-------------------"<<endl<<endl;
}

void Network::Construnt_Current_Topo(Optical *o_db, Lightpath *lp, int **current_topo, float bw, int modulation)
{
	//multi-layer auxiliary graph considering find-grained wavelength layers
	int req_slot_number = ceil((float)bw/(SLOT_CAPACITY*modulation));

	float **ele_capacity = new float *[TOPO_NODE_NUM];
	for (int i = 0; i < TOPO_NODE_NUM; i++)
	{
		ele_capacity[i] = new float[TOPO_NODE_NUM];
		for (int j = 0; j < TOPO_NODE_NUM; j++)
		{
			ele_capacity[i][j] = 0;
		}
	}

	//construct electrical-layer topo
	for (int m = 0; m < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; m++)
	{
		ele_capacity[lp[m].lightpath_src][lp[m].lightpath_dst] += lp[m].lightpath_residual_bw;
	}
	for (int r = 0; r < TOPO_NODE_NUM; r++)
	{
		for (int s = 0; s < TOPO_NODE_NUM; s++)
		{
			if (ele_capacity[r][s] >= bw)
			{
				current_topo[r][s] = ELECTRIC_LINK_WEIGHT;
			}
		}
	}

	//construct optical-layer topo
	for (int y = 0; y < SPECTRUM_SLOT_NUM - req_slot_number + 1; y++)
	{
		for (int k = 0; k < TOPO_LINK_NUM; k++)
		{
			int temp_flag = 0;
			for (int j = 0; j < req_slot_number; j++)
			{
				temp_flag += o_db[k].spectrum_occupy[y+j];
			}

			if (temp_flag == 0)
			{
				current_topo[o_db[k].optical_src + (y+1)*TOPO_NODE_NUM][o_db[k].optical_dst + (y+1)*TOPO_NODE_NUM] = OPTICAL_LINK_WEIGHT + y;
			}
		}
	}

	//construct inter-layer links
	for (int n = 0; n < TOPO_NODE_NUM; n++)
	{
		for (int layer = 1; layer <= SPECTRUM_SLOT_NUM; layer++)
		{
			current_topo[n][n+layer*TOPO_NODE_NUM] = TRANSPONDER_LINK_WEIGHT;
			current_topo[n+layer*TOPO_NODE_NUM][n] = TRANSPONDER_LINK_WEIGHT;
		}
	}

	for (int i = 0; i < TOPO_NODE_NUM; i++)
	{
		delete [] ele_capacity[i];
	}
	delete [] ele_capacity;
}

int Network::Lightpath_Modulation(int lp_length, int cal_mod)
{
	//if higher modulation is available (distance constraint), adopt higher modulation
	if (lp_length <= QAM_16 && cal_mod <= 4)
	{
		return 4;
	}
	else if (lp_length <= QAM_8 && cal_mod <= 3)
	{
		return 3;
	}
	else if (lp_length <= QPSK && cal_mod <= 2)
	{
		return 2;
	}
	else if (lp_length <= BPSK && cal_mod == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void Network::OldNew_Lightpath_Combine(map<int, vector<int>> &split_lp, Lightpath *old_lp, Lightpath *new_lp)
{
	int new_lightpath_control = split_lightpath_control;
	for (int i = 0; i < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; i++)
	{
		//whether split lightpath
		if (split_lp.count(i))
		{
			continue;
		}
		else if (old_lp[i].lightpath_src + old_lp[i].lightpath_dst + old_lp[i].lightpath_modulation + old_lp[i].lightpath_used_slots_num == 0)
		{
			continue;
		}
		else
		{
			new_lp[new_lightpath_control].lightpath_src = old_lp[i].lightpath_src;
			new_lp[new_lightpath_control].lightpath_src = old_lp[i].lightpath_src;
			new_lp[new_lightpath_control].lightpath_dst = old_lp[i].lightpath_dst;
			new_lp[new_lightpath_control].lightpath_modulation = old_lp[i].lightpath_modulation;
			new_lp[new_lightpath_control].lightpath_residual_bw = old_lp[i].lightpath_residual_bw;
			new_lp[new_lightpath_control].lightpath_supporting_bw = old_lp[i].lightpath_supporting_bw;
			new_lp[new_lightpath_control].lightpath_used_slots_num = old_lp[i].lightpath_used_slots_num;
			new_lp[new_lightpath_control].lightpath_used_slots.clear();
			for (int r = 0; r < old_lp[i].lightpath_used_slots.size(); r++)
			{
				new_lp[new_lightpath_control].lightpath_used_slots.push_back(old_lp[i].lightpath_used_slots[r]);
			}
			new_lp[new_lightpath_control].lightpath_path.clear();
			for (int e = 0; e < old_lp[i].lightpath_path.size(); e++)
			{
				new_lp[new_lightpath_control].lightpath_path.push_back(old_lp[i].lightpath_path[e]);
			}
			new_lightpath_control++;
		}
	}
	lightpath_control = new_lightpath_control;
	//cout<<"The number of combined Lightpaths: "<<lightpath_control<<endl;
}

int Network::Path_Resource_Allocation(Optical *optical_db, Lightpath *lp, Traffic *req, int p, vector<int> current_path, float bw, int mod)
{
	int access_flag = 1;
	int start_index = -1;
	int lightpath_dist = 0;
	int req_slot_number = ceil((float)bw/SLOT_CAPACITY);
	int actual_slot_number;
	int new_lightpath_num = 0;
	int new_elepath_num = 0;
	map<int, vector<int>> new_elepath;

	//path resource allocation
	for (int w = 0; w < current_path.size() - 1; w++)
	{
		int layer_decide = current_path[w]/TOPO_NODE_NUM;

		//same layer (specifically, same wavelength layer)
		if (current_path[w]/TOPO_NODE_NUM == current_path[w+1]/TOPO_NODE_NUM)
		{
			//electrical-layer grooming
			if (current_path[w]/TOPO_NODE_NUM == 0)
			{
				float bandwidth = bw;
				for (int r = 0; r < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; r++)
				{
					if (lp[r].lightpath_src == current_path[w] && lp[r].lightpath_dst == current_path[w+1])
					{
						if (lp[r].lightpath_residual_bw >= bandwidth)
						{
							//cout<<"* used lightpath:"<<lp[r].lightpath_src<<"->"<<lp[r].lightpath_dst<<"("<<lp[r].lightpath_residual_bw<<", ";
							lp[r].lightpath_residual_bw -= bandwidth;
							lp[r].lightpath_supporting_bw += bandwidth;
							bandwidth = 0;
							//cout<<"residual capacity"<<lp[r].lightpath_residual_bw<<")"<<endl;
							new_elepath[new_elepath_num].push_back(lp[r].lightpath_src);
							new_elepath[new_elepath_num].push_back(lp[r].lightpath_dst);
							new_elepath_num++;
							req[p].traffic_hops++;
							break;
						}
						else if (lp[r].lightpath_residual_bw > 0)
						{
							//cout<<"used lightpath:"<<lp[r].lightpath_src<<"->"<<lp[r].lightpath_dst<<"("<<lp[r].lightpath_residual_bw<<", ";
							bandwidth -= lp[r].lightpath_residual_bw;
							lp[r].lightpath_supporting_bw += lp[r].lightpath_residual_bw;
							lp[r].lightpath_residual_bw = 0;
							//cout<<"residual capacitys"<<lp[r].lightpath_residual_bw<<")"<<endl;
						}
					}
				}
				//cout<<endl;
				if (bandwidth > 0)
				{
					cout<<"Electric layer bandwidth error"<<endl;
					cout<<"original bandwidth: "<<bw<<endl;
					cout<<"Bandwidth: "<<bandwidth<<endl;
					for (int i = 0; i < current_path.size(); i++)
					{
						cout<<current_path[i]<<" ";
					}
					cout<<endl;
				}
				continue;
			}

			//continue current lightpath on optical layer
			else
			{
				for (int c = 0; c < TOPO_LINK_NUM; c++)
				{
					if (optical_db[c].optical_src == current_path[w] % TOPO_NODE_NUM && optical_db[c].optical_dst == current_path[w+1] % TOPO_NODE_NUM)
					{
						//cout<<" used optical link:"<<optical_database[c].optical_src<<"->"<<optical_database[c].optical_dst<<endl;
						lp[lightpath_control].lightpath_path.push_back(optical_db[c].optical_dst);
						lightpath_dist += optical_db[c].optical_length;
						break;
					}
				}
			}
		}

		//cross layer
		else
		{
			//Transponder link
			if (current_path[w] % TOPO_NODE_NUM == current_path[w+1] % TOPO_NODE_NUM && current_path[w]/TOPO_NODE_NUM == 0) //outbound node is on electrical layer
			{
				//cout<<"* start a new lightpath"<<endl;
				start_index = w;
				lp[lightpath_control].lightpath_path.clear();
				lp[lightpath_control].lightpath_path.push_back(current_path[w] % TOPO_NODE_NUM);
				lightpath_dist = 0;
				actual_slot_number = req_slot_number;
				continue;
			}
			else if (current_path[w] % TOPO_NODE_NUM == current_path[w+1] % TOPO_NODE_NUM && current_path[w+1]/TOPO_NODE_NUM == 0) //inbound node is on electrical layer
			{
				if (Lightpath_Modulation(lightpath_dist, mod))
				{
					(lp+lightpath_control)->lightpath_modulation = Lightpath_Modulation(lightpath_dist, mod);
					actual_slot_number = ceil((float) req_slot_number/((lp+lightpath_control)->lightpath_modulation));
				}
				else
				{
					//cout<<"Error! path is too long to assign a modulation format"<<endl;
					lp[lightpath_control].lightpath_path.clear();
					access_flag = 0;
				}

				if (access_flag == 1)
				{
					cout<<"finishing a new lightpath establishment"<<current_path[start_index]%TOPO_NODE_NUM<<"->"<<current_path[w]%TOPO_NODE_NUM<<", length:"<<lightpath_dist<<", modulaiton:"<<(lp+lightpath_control)->lightpath_modulation<<endl;
					for (int b = 0; b < (lp+lightpath_control)->lightpath_path.size()-1; b++)
					{
						for (int c = 0; c < TOPO_LINK_NUM; c++)
						{
							if (optical_db[c].optical_src == (lp+lightpath_control)->lightpath_path[b] && optical_db[c].optical_dst == (lp+lightpath_control)->lightpath_path[b+1])
							{
								//cout<<" used fiber"<<optical_db[c].optical_src<<"->"<<optical_db[c].optical_dst<<", spectrum: ";
								for (int st = 0; st < actual_slot_number; st++)
								{
									optical_db[c].spectrum_occupy[st + layer_decide - 1] = 1;
									//cout<<st + layer_decide - 1<<" ";
								}

								if ((lp+lightpath_control)->lightpath_used_slots.size() == 0)
								{
									for (int e = 0; e < actual_slot_number; e++)
									{
										(lp+lightpath_control)->lightpath_used_slots.push_back(e + layer_decide - 1);
									}
								}
								(lp+lightpath_control)->lightpath_used_slots_num = (lp+lightpath_control)->lightpath_used_slots.size();
								//cout<<"spectrum width"<<(lp+lightpath_control)->lightpath_used_slots_num<<endl;
								break;
							}
						}
					}

					(lp + lightpath_control)->lightpath_src = current_path[start_index]%TOPO_NODE_NUM;
					(lp + lightpath_control)->lightpath_dst = current_path[w]%TOPO_NODE_NUM;
					(lp + lightpath_control)->lightpath_residual_bw = SLOT_CAPACITY*actual_slot_number*(lp + lightpath_control)->lightpath_modulation - bw;
					(lp + lightpath_control)->lightpath_supporting_bw = bw; 
					//cout<<"lightpath residual bandwidth:"<<(lp + lightpath_control)->lightpath_residual_bw<<", path��";
					//for (int r = 0; r < (lp + lightpath_control)->lightpath_path.size(); r++)
					//{
						//cout<<(lp + lightpath_control)->lightpath_path[r]<<" ";
					//}
					//cout<<endl;
					lightpath_control++; //existing lightpath number
					//cout<<"current Lightpathm number: "<<lightpath_control<<endl<<endl;
					new_lightpath_num++;
					req[p].traffic_hops++;
					continue;
				}

				else
				{
					if (new_lightpath_num)
					{
						//cout<<"traffic failed! tear down current lightpath"<<": ";
						for (int i = 0; i < new_lightpath_num; i++)
						{
							//cout<<lp[lightpath_control-1].lightpath_src<<"->"<<lp[lightpath_control-1].lightpath_dst<<endl;
							lp[lightpath_control-1].lightpath_src = 0;
							lp[lightpath_control-1].lightpath_dst = 0;
							lp[lightpath_control-1].lightpath_modulation = 0;
							lp[lightpath_control-1].lightpath_residual_bw = 0;
							lp[lightpath_control-1].lightpath_supporting_bw = 0;
							lp[lightpath_control-1].lightpath_used_slots_num = 0;
							lp[lightpath_control-1].lightpath_used_slots.clear();
							lp[lightpath_control-1].lightpath_path.clear();
							lightpath_control--;
						}
						//cout<<endl;
						req[p].traffic_hops = 0;
						break;
					}
					else if (new_elepath_num)
					{
						//cout<<"traffic failed! tear down current lightpath"<<": ";
						for (int r = 0; r < new_elepath_num; r++)
						{
							//cout<<new_elepath[r][0]<<"->"<<new_elepath[r][1]<<"(";
							for (int s = 0; s < lightpath_control; s++)
							{
								if (lp[s].lightpath_src == new_elepath[r][0] && lp[s].lightpath_dst == new_elepath[r][1])
								{
									//cout<<lp[s].lightpath_residual_bw<<", ";
									lp[s].lightpath_residual_bw += bw;
									lp[s].lightpath_supporting_bw -= bw;
									//cout<<lp[s].lightpath_residual_bw<<")"<<endl;
									break;
								}
							}
						}
						req[p].traffic_hops = 0;
					}
					else
					{
						//cout<<"traffic failed!"<<endl;
						req[p].traffic_hops = 0;
						break;
					}
				}
			}
			else
			{
				cout<<"Error"<<endl;
			}
		}
	}

	if (lp[lightpath_control-1].lightpath_path[0] != lp[lightpath_control-1].lightpath_src)
	{
		cout<<"Lightpath resource allocation error!"<<endl;
	}
	if (lp[lightpath_control-1].lightpath_path[lp[lightpath_control-1].lightpath_path.size()-1] != lp[lightpath_control-1].lightpath_dst)
	{
		cout<<"Lightpath resource allocation error!"<<endl;
	}

	return access_flag;
}

void Network::Routing_and_Resource_Allocation(Optical *optical_db, Lightpath *lp, Traffic *req, int traffic_type)
{
	int **current_topo = new int *[TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM)];
	for (int t = 0; t < TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM); t++)
	{
		current_topo[t] = new int[TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM)];
	}

	for (int p = 0; p < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); p++)
	{
		cout<<endl<<"===================="<<endl;
		if (traffic_type == 0)
		{
			cout<<"Baseline accommodation No. "<<p<<" traffic "<<req[p].traffic_src<<"->"<<req[p].traffic_dst<<", bandwidth:"<<req[p].traffic_bw<<endl;
		}
		else if (traffic_type == 1)
		{
			cout<<"Incremental accommodation No. "<<p<<" traffic "<<req[p].traffic_src<<"->"<<req[p].traffic_dst<<", bandwidth:"<<req[p].traffic_bw<<endl;
		}
		//else
		//{
			//cout<<"Error traffic type"<<endl;
			//return;
		//}
		float flexible_bw = req[p].traffic_bw;
		vector<int> current_path;
		int mod; //modulation level for path calculation

		while (flexible_bw > 0)
		{
			for (mod = 1; mod < 5; mod++)
			{
				//initialize topo
				for (int t = 0; t < TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM); t++)
				{
					for (int r = 0; r < TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM); r++)
					{
						current_topo[t][r] = MAX;
					}
				}
				Construnt_Current_Topo(optical_db, lp, current_topo, flexible_bw, mod);
				current_path = dijkstra(current_topo, req[p].traffic_src, req[p].traffic_dst, TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM));
				if (current_path.size() > 0)
				{
					break;
				}
			}
			//no path
			if (current_path.size() == 0)
			{
				if (traffic_type == 0) //Baseline no path, failure
				{
					req[p].traffic_actual_bw = 0;
					req[p].traffic_success = 0;
					break;
				}
				else if (traffic_type == 1) //Incremental no path. re-calculate path with a lower bandwidth
				{
					flexible_bw -= MU;
					continue;
				}
			} 
			//there is a path
			else 
			{
				int longest_lightpath_length = 0;
				int current_lightpath_length;
				for (int ff = 0; ff < current_path.size()-1; ff++)
				{
					//same layer
					if (current_path[ff]/TOPO_NODE_NUM == current_path[ff+1]/TOPO_NODE_NUM) //
					{
						//same electrical layer
						if (current_path[ff]/TOPO_NODE_NUM == 0)
						{
							continue;
						}
						//same optical layer
						else
						{
							for (int c = 0; c < TOPO_LINK_NUM; c++)
							{
								if (optical_db[c].optical_src == current_path[ff] % TOPO_NODE_NUM && optical_db[c].optical_dst == current_path[ff+1] % TOPO_NODE_NUM)
								{
									current_lightpath_length += optical_db[c].optical_length;
									break;
								}
							}
						}
					}
					//cross layer
					else
					{
						if (current_path[ff] % TOPO_NODE_NUM == current_path[ff+1] % TOPO_NODE_NUM && current_path[ff]/TOPO_NODE_NUM == 0) //outbound node is on electrical layer
						{
							current_lightpath_length = 0;
						}
						else if (current_path[ff] % TOPO_NODE_NUM == current_path[ff+1] % TOPO_NODE_NUM && current_path[ff+1]/TOPO_NODE_NUM == 0) //inbound node is on electrical layer
						{
							if (current_lightpath_length > longest_lightpath_length)
							{
								longest_lightpath_length = current_lightpath_length;
							}
						}
						else
						{
							cout<<"Error!!"<<endl;
						}
					}
				}

				if (traffic_type == 0) //Baseline, there is a path
				{
					//cout<<"modualtion format for path calculation: "<<mod;
					//cout<<", bandwidth for path calculation: "<<flexible_bw<<endl;
					if (Lightpath_Modulation(longest_lightpath_length, mod))
					{
						Path_Resource_Allocation(optical_db, lp, req, p, current_path, flexible_bw, mod);
						req[p].traffic_actual_bw = flexible_bw;
						req[p].traffic_success = 1;
						overall_traffic_success++;
					}
					else
					{
						//cout<<"Path acquired, but lightpaths are too long to allocate resource"<<endl;
						req[p].traffic_actual_bw = 0;
						req[p].traffic_success = 0;
					}
					break;
				}

				else //Incremental, there is a path
				{
					//cout<<"modualtion format for path calculation: "<<mod;
					//cout<<", bandwidth for path calculation: "<<flexible_bw<<endl;
					if (Lightpath_Modulation(longest_lightpath_length, mod))
					{
						Path_Resource_Allocation(optical_db, lp, req, p, current_path, flexible_bw, mod);
						//for (int f = 0; f < current_path.size(); f++)
						//{
							//cout<<current_path[f]<<" ";
						//}
						//cout<<endl;
						req[p].traffic_actual_bw = flexible_bw;
						req[p].traffic_success = 1;
						overall_traffic_success++;
						break;
					}
					else
					{
						//cout<<"Path acquired, but lightpaths are too long to allocate resource"<<endl;
						req[p].traffic_actual_bw = 0;
						req[p].traffic_success = 0;
						flexible_bw -= MU;
						continue;
					}
				}
			}
		}
		if (flexible_bw == 0 || current_path.size() == 0)
		{
			//cout<<"calculation failure"<<endl;
			req[p].traffic_actual_bw = 0;
			req[p].traffic_success = 0;
		}
		//cout<<"number of traffic hops on electrical layer: "<<req[p].traffic_hops<<endl;
	}

	for (int x = 0; x < TOPO_NODE_NUM*(1+SPECTRUM_SLOT_NUM); x++)
	{
		delete [] current_topo[x];
	}
	delete [] current_topo;

	return;
}

void Network::Generate_Baseline_Configurations()
{
	lightpath_database = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];

	Routing_and_Resource_Allocation(optical_database, lightpath_database, baseline_traffic, 0);
	
	float baseline_throughput = 0;
	int access_no = 0;
	int total_hops = 0;
	float average_hops;
	for (int x = 0; x < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); x++)
	{
		baseline_throughput += baseline_traffic[x].traffic_success*baseline_traffic[x].traffic_bw;
		access_no += baseline_traffic[x].traffic_success;
		total_hops += baseline_traffic[x].traffic_hops*baseline_traffic[x].traffic_bw;
	}
	average_hops = (float)total_hops/baseline_throughput;
	
	cout<<endl<<"Baseline successful traffic number: "<<access_no<<", current number of Lightpaths: "<<lightpath_control<<endl;
	cout<<"*Baseline throughput: "<<baseline_throughput<<" (*6.25)Gbps"<<endl;
	cout<<"*average bps traffic hops: "<<average_hops<<endl<<endl;
	
	baseline_lightpath_control = lightpath_control;
	baseline_overall_traffic_success = overall_traffic_success;
}

void Network::General_Incremental_Routing()
{	
	Routing_and_Resource_Allocation(optical_database, lightpath_database, incremental_traffic, 1);
	
	float incremental_throughput = 0;
	int incre_access_no = 0;
	int total_hops = 0;
	float average_hops;
	int possible_splitpoints = 0;

	for (int x = 0; x < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); x++)
	{
		incremental_throughput += incremental_traffic[x].traffic_success*incremental_traffic[x].traffic_actual_bw;
		incre_access_no += incremental_traffic[x].traffic_success;
		total_hops += incremental_traffic[x].traffic_hops*incremental_traffic[x].traffic_actual_bw;
	}

	for (int y = 0; y < lightpath_control; y++)
	{
		if (lightpath_database[y].lightpath_modulation < 4)
		{
			possible_splitpoints += lightpath_database[y].lightpath_path.size()-2;
		}
	}
	average_hops = (float)total_hops/incremental_throughput;
	cout<<endl<<"Incremental successful traffic number: "<<incre_access_no<<", current number of Lightpaths: "<<lightpath_control<<", possible splitpoints number: "<<possible_splitpoints<<endl;
	cout<<"*Incremental throughput: "<<incremental_throughput<<" (*6.25)Gbps"<<endl;
	cout<<"*average bps traffic hops: "<<average_hops<<endl<<endl;

	pre_split_lightpath_control = lightpath_control;
	pre_split_overall_traffic_success = incre_access_no;

	//////////////////////////////////////////////////////////////////////////
	//post-split traffic requests
	incremental_residual_traffic = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];
	for (int i = 0; i < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); i++)
	{
		incremental_residual_traffic[i].traffic_ID = incremental_traffic[i].traffic_ID;
		incremental_residual_traffic[i].traffic_src = incremental_traffic[i].traffic_src;
		incremental_residual_traffic[i].traffic_dst = incremental_traffic[i].traffic_dst;
		incremental_residual_traffic[i].traffic_bw = incremental_traffic[i].traffic_bw - incremental_traffic[i].traffic_actual_bw; //residual bandwidth for next-step routing
		incremental_residual_traffic[i].traffic_actual_bw = 0;
		incremental_residual_traffic[i].traffic_success = 0;
	}
	bubble_sort(incremental_residual_traffic, TOPO_NODE_NUM*(TOPO_NODE_NUM-1));
}

void Network::Greedy_Target_LP_Selection(int splitpoint_num, int greedy_option, map<int, int> &target_result, map<int, int> &target_lp_splitnum, Traffic *sorted_traffic_residual, Lightpath *lp, Optical *optical_db)
{
	//here we assume that all lightpaths have infinity capacity. traffic spikes is served, and we observe the supporting bandwidth of each lightpath to perform the greedy algorithms
	for (int uu = 0; uu < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; uu++)
	{
		lp[uu].lightpath_residual_bw = MAX;
	}

	int temp_1 = lightpath_control;
	int temp_2 = overall_traffic_success;
	Routing_and_Resource_Allocation(optical_db, lp, sorted_traffic_residual, 1);
	lightpath_control = temp_1;
	overall_traffic_success = temp_2;

	map<int, int> old_index;
	for (int u = 0; u < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; u++)
	{
		old_index[u] = u;
	}
	bubble_sort_lp(lp, old_index, TOPO_LINK_NUM*SPECTRUM_SLOT_NUM);

	map<int, int> current_lp_hops;

	//split as many lightpaths as possible
	if (greedy_option == 0)
	{
		int k_count = 0;
		for (int k = 0; k < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; k++)
		{
			if (old_index[k] >= lightpath_control)
			{
				continue; //the selected lightpath must be one of the lightpath in original lp
			}
			if (lp[k].lightpath_path.size() > 2 && lp[k].lightpath_modulation < 4)  //can be split
			{
				target_result[k_count] = old_index[k]; //the lightpath index of the lp
				target_lp_splitnum[k_count] = 1;
				current_lp_hops[k_count] = lp[k].lightpath_path.size();
				k_count++;
			}
			if (k_count >= splitpoint_num)
			{
				break;
			}
		}

		int accumulate_k_count = k_count;
		
		//if after a round of selection, the selected number of splitpoints is smaller than the given splitpoint_num, then, we have another round of selection
		if (k_count < splitpoint_num)
		{
			for (int s = 0; s < k_count; s++)
			{
				if (accumulate_k_count < splitpoint_num)
				{
					if (accumulate_k_count + current_lp_hops[s] - 3 < splitpoint_num)
					{
						target_lp_splitnum[s] = current_lp_hops[s] - 2; //assign the largest splitpoints number
						accumulate_k_count += current_lp_hops[s] - 3;
					}
					else
					{
						target_lp_splitnum[s] = 1 + splitpoint_num - accumulate_k_count;
						accumulate_k_count = splitpoint_num;

					}
					continue;
				}
				else
				{
					break;
				}
			}
		}
	} 
	//split one lightpath as much as possible
	else
	{
		int k_count = 0;
		int d = 0;
		
		for (int k = 0; k < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; k++)
		{
			if (old_index[k] >= lightpath_control)
			{
				continue; //the selected lightpath must be one of the lightpath in original lp
			}
			if (lp[k].lightpath_path.size() > 2 && lp[k].lightpath_modulation < 4)  //can be split
			{
				if (lp[k].lightpath_path.size() - 2 > splitpoint_num - k_count)
				{
					target_result[d] = old_index[k]; //the lightpath index of the lp
					target_lp_splitnum[d] = splitpoint_num - k_count;
					k_count += splitpoint_num - k_count;
					d++;
				}
				else
				{
					target_result[d] = old_index[k];
					target_lp_splitnum[d] = lp[k].lightpath_path.size() - 2;
					k_count += lp[k].lightpath_path.size() - 2;
					d++;
				}
			}
			if (k_count >= splitpoint_num)
			{
				break;
			}
		}
	}
}

void Network::Combine(int n, int m, int a[], int b[], const int M, map<int, vector<int>> &result)
{
	for(int j = n; j >= m; j--)
	{
		b[m-1] = j-1;
		if(m > 1)
		{
			Combine(j-1, m-1, a, b, M, result); //recursive
		}
		else
		{
			for(int i = M-1;i >= 0; i--)
			{
				result[combine_control].push_back(a[b[i]]);
			}
			combine_control++;
		}
	}
}

vector<int> Network::Greedy_Splitpoint_Determination(int greedy_option, Lightpath *lp, int lp_num, int sp_num)
{
	map<int, vector<int>> possible_splitpoints;
	vector<int> splitpoints;
	int n = lp[lp_num].lightpath_path.size() - 2;
	int m = sp_num;

	int *a = new int[n];
	int *b = new int[m];
	for(int i = 0; i < n; i++)
	{
		a[i] = i + 1;
	}
	const int M = m;
	combine_control = 0;
	Combine(n, m, a, b, M, possible_splitpoints);

	int temp_e_capacity = 0;
	int temp_o_occupy = MAX;
	
	//try every possible splitpoints combinations
	for (int j = 0; j < possible_splitpoints.size(); j++) 
	{
		//initialize splitpoints of lightpath
		int current_e_capacity = 0; 
		int current_o_occupy = 0;
		int current_distance = 0;
		for (int p = 1; p < lp[lp_num].lightpath_path.size(); p++)
		{
			for (int d = 0; d < TOPO_LINK_NUM; d++)
			{
				if (optical_database[d].optical_src == lp[lp_num].lightpath_path[p-1] && optical_database[d].optical_dst == lp[lp_num].lightpath_path[p])
				{
					current_distance += optical_database[d].optical_length;
				}
			}

			int temp_flag = 0;
			for (int w = 0; w < possible_splitpoints[j].size(); w++)
			{
				if (p == possible_splitpoints[j][w])
				{
					temp_flag = 1;
					break;
				}
			}

			//check whether the node is splitpoints or terminating node
			if (temp_flag == 1 || p == lp[lp_num].lightpath_path.size()-1)
			{
				current_e_capacity += lp[lp_num].lightpath_used_slots_num*Lightpath_Modulation(current_distance, 1);
				current_o_occupy += ceil((float)lp[lp_num].lightpath_used_slots_num*lp[lp_num].lightpath_modulation/Lightpath_Modulation(current_distance, 1));
				current_distance = 0;
			}
			else
			{
				continue;
			}
		}
		
		if (greedy_option == 0)  //max electric capacity
		{
			if (current_e_capacity > temp_e_capacity)
			{
				temp_e_capacity = current_e_capacity;
				splitpoints.clear();
				for (int y = 0; y < possible_splitpoints[j].size(); y++)
				{
					splitpoints.push_back(possible_splitpoints[j][y]);
				}
			}
		} 
		else //max optical capacity
		{
			if (current_o_occupy < temp_o_occupy)
			{
				temp_o_occupy = current_o_occupy;
				splitpoints.clear();
				for (int y = 0; y < possible_splitpoints[j].size(); y++)
				{
					splitpoints.push_back(possible_splitpoints[j][y]);
				}
			}
		}
	}
	delete [] a;
	delete [] b;
	bubble(splitpoints, sp_num); //sorting
	return splitpoints;
}

//split the No. 'splitpoints' point on No. 't' lightpath of the given 'lp' set
int Network::Split_Lightpath(int t, vector<int> splitpoints, Optical *optical_db, Lightpath *lp, Lightpath *new_lp, int greedy_option_B)
{
	//this function split #t lightpath of *lp into *new_lp
	int split_flag;

	//if this lightpath is one hop, or already the highest modualtion, then it cannot be split
	if (lp[t].lightpath_path.size() == 2 || lp[t].lightpath_modulation == 4)
	{
		cout<<"Error"<<endl<<"--"<<endl;
		cout<<"Path size: "<<lp[t].lightpath_path.size()<<", modulation: "<<lp[t].lightpath_modulation<<endl;
		cout<<"["<<t<<"] "<<lp[t].lightpath_src<<"->"<<lp[t].lightpath_dst<<" ����split"<<endl;
		new_lp[split_lightpath_control].lightpath_src = lp[t].lightpath_src;
		new_lp[split_lightpath_control].lightpath_dst = lp[t].lightpath_dst;
		new_lp[split_lightpath_control].lightpath_modulation = lp[t].lightpath_modulation;
		new_lp[split_lightpath_control].lightpath_residual_bw = lp[t].lightpath_residual_bw;
		new_lp[split_lightpath_control].lightpath_used_slots_num = lp[t].lightpath_used_slots_num;
		new_lp[split_lightpath_control].lightpath_path.clear();
		for (int y = 0; y < lp[t].lightpath_path.size(); y++)
		{
			new_lp[split_lightpath_control].lightpath_path.push_back(lp[t].lightpath_path[y]);
		}
		for (int r = 0; r < lp[t].lightpath_used_slots.size(); r++)
		{
			new_lp[split_lightpath_control].lightpath_used_slots.push_back(lp[t].lightpath_used_slots[r]);
		}
		split_lightpath_control++;
		split_flag = 0;		
	}

	//this lightpath can be split
	else if (lp[t].lightpath_path.size() > 2)
	{
		if (lp[t].lightpath_path.size() < splitpoints.size() + 2)
		{
			cout<<"splitpoints number Error!"<<endl;
			cout<<"Lightpath size: "<<lp[t].lightpath_path.size()<<": ";
			for (int i = 0; i < lp[t].lightpath_path.size(); i++)
			{
				cout<<lp[t].lightpath_path[i]<<" ";
			}
			cout<<endl;
			cout<<"splitpoint number: "<<splitpoints.size()<<": ";
			for (int i = 0; i < splitpoints.size(); i++)
			{
				cout<<splitpoints[i]<<" ";
			}
			cout<<endl;
			
		}
		else //do the split operation
		{
			//cout<<"--"<<endl;
			//cout<<"["<<t<<"] "<<lp[t].lightpath_src<<"->"<<lp[t].lightpath_dst<<"split, moduation"<<lp[t].lightpath_modulation<<", residual bandwidth: "<<lp[t].lightpath_residual_bw;
			//cout<<", spectrum width"<<lp[t].lightpath_used_slots_num<<": ";
			//for (int h = 0; h < lp[t].lightpath_used_slots.size(); h++)
			//{
				//cout<<lp[t].lightpath_used_slots[h]<<" ";
			//}
			//cout<<endl;

			//release the occupied spectrum slots
			for (int w = 0; w < lp[t].lightpath_path.size()-1; w++)
			{
				for (int v = 0; v < TOPO_LINK_NUM; v++)
				{
					if (optical_db[v].optical_src == lp[t].lightpath_path[w] && optical_db[v].optical_dst == lp[t].lightpath_path[w+1])
					{
						for (int s = 0; s < lp[t].lightpath_used_slots.size(); s++)
						{
							optical_db[v].spectrum_occupy[lp[t].lightpath_used_slots[s]] = 0;
						}
						break;
					}
				}
			}
			
			//start spliting the points in 'splitpoints'
			int start_point = lp[t].lightpath_src;
			splitpoints.push_back(lp[t].lightpath_path.size()-1);
			for (int split = 0; split < splitpoints.size(); split++)
			{
				int dist = 0;
				//cout<<"No. "<<split+1<<" new lightpath: ";
				new_lp[split_lightpath_control].lightpath_src = start_point;
				new_lp[split_lightpath_control].lightpath_dst = lp[t].lightpath_path[splitpoints[split]];
				//cout<<new_lp[split_lightpath_control].lightpath_src<<"->"<<new_lp[split_lightpath_control].lightpath_dst<<" ";

				new_lp[split_lightpath_control].lightpath_path.clear();
				int temp_flag = 0;
				for (int y = 0; y < lp[t].lightpath_path.size(); y++)
				{
					if (lp[t].lightpath_path[y] == new_lp[split_lightpath_control].lightpath_src)
					{
						temp_flag = 1;
					}
					if (temp_flag == 1)
					{
						new_lp[split_lightpath_control].lightpath_path.push_back(lp[t].lightpath_path[y]);
					}
					if (lp[t].lightpath_path[y] == new_lp[split_lightpath_control].lightpath_dst)
					{
						temp_flag = 0;
						break;
					}
				}

				for (int x = 0; x < new_lp[split_lightpath_control].lightpath_path.size() - 1; x++)
				{
					for (int ii = 0; ii < TOPO_LINK_NUM; ii++)
					{
						if (optical_db[ii].optical_src == new_lp[split_lightpath_control].lightpath_path[x] && optical_db[ii].optical_dst == new_lp[split_lightpath_control].lightpath_path[x+1])
						{
							dist += optical_db[ii].optical_length;
							break;
						}
					}
				}
				if (Lightpath_Modulation(dist, 1))
				{
					new_lp[split_lightpath_control].lightpath_modulation = Lightpath_Modulation(dist, 1);
					if (greedy_option_B == 0) //max electric capacity
					{
						new_lp[split_lightpath_control].lightpath_used_slots_num = lp[t].lightpath_used_slots_num;
					}
					else
					{
						new_lp[split_lightpath_control].lightpath_used_slots_num = ceil((float)lp[t].lightpath_modulation*lp[t].lightpath_used_slots_num/Lightpath_Modulation(dist, 1));
					}
				}
				else
				{
					cout<<"Error!"<<endl;
				}

				//cout<<"optical layer path: ";
				//for (int i = 0; i < new_lp[split_lightpath_control].lightpath_path.size(); i++)
				//{
					//cout<<new_lp[split_lightpath_control].lightpath_path[i]<<" ";
				//}
				//cout<<endl<<"new modulation format: "<<new_lp[split_lightpath_control].lightpath_modulation<<" ";
				//cout<<new spectrum width: "<<new_lp[split_lightpath_control].lightpath_used_slots_num<<" ";
				new_lp[split_lightpath_control].lightpath_residual_bw = lp[t].lightpath_residual_bw + new_lp[split_lightpath_control].lightpath_used_slots_num*new_lp[split_lightpath_control].lightpath_modulation*SLOT_CAPACITY - lp[t].lightpath_modulation*lp[t].lightpath_used_slots_num*SLOT_CAPACITY;
				//cout<<"new residual bandwidth: "<<new_lp[split_lightpath_control].lightpath_residual_bw<<endl;

				//Splitted lightpath spectrum allocation
				//Unsplitted unchanged, splitted first fit
				//max optical available, max electric capacity do not affect spectrum allocations
				if (greedy_option_B != 0)
				{
					new_lp[split_lightpath_control].lightpath_used_slots.clear();
					for (int i = 0; i < SPECTRUM_SLOT_NUM - new_lp[split_lightpath_control].lightpath_used_slots_num + 1; i++)
					{
						int temp_flag = 0;
						int jump = 0;
						for (int y = 0; y < new_lp[split_lightpath_control].lightpath_used_slots_num && jump == 0; y++)
						{
							for (int j = 0; j < new_lp[split_lightpath_control].lightpath_path.size()-1 && jump == 0; j++)
							{
								for (int m = 0; m < TOPO_LINK_NUM; m++)
								{
									if (optical_db[m].optical_src == new_lp[split_lightpath_control].lightpath_path[j] && optical_db[m].optical_dst == new_lp[split_lightpath_control].lightpath_path[j+1])
									{
										temp_flag += optical_db[m].spectrum_occupy[i+y];
										if (temp_flag > 0)
										{
											jump = 1;
										}
										break;
									}
								}
							}
						}

						//assign spectrum slots to new lightpaths
						if (temp_flag == 0)
						{
							//we find the splitted lightpath location on spectrum
							//cout<<"post split"<<new_lp[split_lightpath_control].lightpath_src<<"->"<<new_lp[split_lightpath_control].lightpath_dst<<"spectrum: ";
							for (int r = 0; r < new_lp[split_lightpath_control].lightpath_used_slots_num; r++)
							{
								new_lp[split_lightpath_control].lightpath_used_slots.push_back(i+r);
								//cout<<i+r<<" ";
								for (int j = 0; j < new_lp[split_lightpath_control].lightpath_path.size()-1; j++)
								{
									for (int g = 0; g < TOPO_LINK_NUM; g++)
									{
										if (optical_db[g].optical_src == new_lp[split_lightpath_control].lightpath_path[j] && optical_db[g].optical_dst == new_lp[split_lightpath_control].lightpath_path[j+1])
										{
											optical_db[g].spectrum_occupy[i+r] = 1;
											break;
										}
									}
								}
							}
							//cout<<endl;
							break;
						}	
					}
				}
				split_lightpath_control++;
				start_point = lp[t].lightpath_path[splitpoints[split]];
			}
			split_flag = 1;
		}		
	}
	else
	{
		cout<<"--"<<endl;
		cout<<"Split Lightpath Size Error!"<<endl;
		cout<<"Size = "<<new_lp[t].lightpath_path.size()<<endl;
		return 0;
	}
	//cout<<"current New Lightpath number: "<<split_lightpath_control<<endl;
	return split_flag;
}

void Network::Greedy_Lightpath_Splitting(int splitpoint_num, int greedy_option_A, int greedy_option_B, Optical *o_db, Lightpath *lp_db, Traffic *req)
{
	lightpath_control = pre_split_lightpath_control;
	overall_traffic_success = pre_split_overall_traffic_success;
	split_lightpath_control = 0;
	
	//////////////////////////////////////////////////////////////////////////
	//temp database for lightpath selection
	Optical *temp_optical_database = new Optical[TOPO_LINK_NUM];
	Lightpath *temp_lightpath_database = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	Traffic *temp_req = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];	
	Copy_Optical_Database(o_db, temp_optical_database);	
	Copy_Lightpath_Database(lp_db, temp_lightpath_database);
	Copy_Traffic_Database(req, temp_req);
	
	map<int, int> target_lightpath; //target lightpath set
	map<int, int> target_lp_splitnum; //target lightpath splitpoint number

	Greedy_Target_LP_Selection(splitpoint_num, greedy_option_A, target_lightpath, target_lp_splitnum, temp_req, temp_lightpath_database, temp_optical_database);

	for (int i = 0; i < TOPO_LINK_NUM; i++)
	{
		delete [] temp_optical_database[i].spectrum_occupy;
	}
	delete [] temp_optical_database;
	delete [] temp_lightpath_database;
	delete [] temp_req;
	
	//////////////////////////////////////////////////////////////////////////
	//base for split and post-split resource allocation
	Optical *temp_optical_database_postsplit = new Optical[TOPO_LINK_NUM];
	Lightpath *temp_lightpath_database_postsplit = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	Traffic *temp_req_postsplit = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];
	Copy_Optical_Database(o_db, temp_optical_database_postsplit);	
	Copy_Lightpath_Database(lp_db, temp_lightpath_database_postsplit);
	Copy_Traffic_Database(req, temp_req_postsplit);

	float affected_bandwidth = 0;
	float increased_hops_bw = 0;
	Lightpath *lightpath_database_split_greedy_result = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	map<int, vector<int>> split_lp_and_points;
	//decide how to split every lightpath according to target_lightpath
	for (int r = 0; r < target_lightpath.size(); r++)
	{
		//need to selected target_lp_splitnum[r] splitpoints
		if (temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_path.size()-2 > target_lp_splitnum[r])
		{
			split_lp_and_points[target_lightpath[r]] = Greedy_Splitpoint_Determination(greedy_option_B, temp_lightpath_database_postsplit, target_lightpath[r], target_lp_splitnum[r]);
			Split_Lightpath(target_lightpath[r], split_lp_and_points[target_lightpath[r]], temp_optical_database_postsplit, temp_lightpath_database_postsplit, lightpath_database_split_greedy_result, greedy_option_B);
			affected_bandwidth += temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_supporting_bw;
			increased_hops_bw += temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_supporting_bw * split_lp_and_points[target_lightpath[r]].size();
		}
		//all intermediate points are splitpoints
		else
		{
			for (int y = 1; y < temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_path.size()-1; y++)
			{
				split_lp_and_points[target_lightpath[r]].push_back(y);
			}
			Split_Lightpath(target_lightpath[r], split_lp_and_points[target_lightpath[r]], temp_optical_database_postsplit, temp_lightpath_database_postsplit, lightpath_database_split_greedy_result, greedy_option_B);	
			affected_bandwidth += temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_supporting_bw;
			increased_hops_bw += temp_lightpath_database_postsplit[target_lightpath[r]].lightpath_supporting_bw * split_lp_and_points[target_lightpath[r]].size();
		}
	}
	OldNew_Lightpath_Combine(split_lp_and_points, temp_lightpath_database_postsplit, lightpath_database_split_greedy_result);
	Routing_and_Resource_Allocation(temp_optical_database_postsplit, lightpath_database_split_greedy_result, temp_req_postsplit, 1);

	float incremental_postsplit_throughput = 0;
	int incre_postsplit_access_no = 0;
	int total_hops = 0;
	float average_hops;

	for (int x = 0; x < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); x++)
	{
		incremental_postsplit_throughput += temp_req_postsplit[x].traffic_success*temp_req_postsplit[x].traffic_actual_bw;
		incre_postsplit_access_no += temp_req_postsplit[x].traffic_success;
		total_hops += temp_req_postsplit[x].traffic_hops*temp_req_postsplit[x].traffic_actual_bw;
	}
	average_hops = (float)total_hops/incremental_postsplit_throughput;
	//cout<<endl<<"number of accommodated traffic: "<<incre_postsplit_access_no<<", current Lightpath number: "<<lightpath_control<<endl;
	//cout<<"*Incremental with Splitting throughput: "<<incremental_postsplit_throughput<<" (*6.25)Gbps"<<endl;
	//cout<<"*Affected bandwidth: "<<affected_bandwidth<<endl;
	//cout<<"*average bps traffic hops: "<<average_hops<<endl<<endl;
	//cout<<"*throughput, Affected bandwidth, average bps traffic hops"<<endl;
	cout<<incremental_postsplit_throughput<<" "<<affected_bandwidth<<" "<<average_hops<<" "<<increased_hops_bw<<endl;
	
	//for (int i = 0; i < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; i++)
	//{
		//if (lightpath_database_split_greedy_result[i].lightpath_src + lightpath_database_split_greedy_result[i].lightpath_dst + lightpath_database_split_greedy_result[i].lightpath_modulation + lightpath_database_split_greedy_result[i].lightpath_used_slots_num + lightpath_database_split_greedy_result[i].lightpath_residual_bw)
		//{
			//cout<<"--"<<endl;
			//cout<<"["<<i<<"] "<<lightpath_database_split_greedy_result[i].lightpath_src<<"->"<<lightpath_database_split_greedy_result[i].lightpath_dst<<", modulation format: "<<lightpath_database_split_greedy_result[i].lightpath_modulation<<", residual bandwidth: "<<lightpath_database_split_greedy_result[i].lightpath_residual_bw<<", used spectrum: "<<lightpath_database_split_greedy_result[i].lightpath_used_slots_num<<"number: ";
			//for (int j = 0; j < lightpath_database_split_greedy_result[i].lightpath_used_slots.size(); j++)
			//{
				//cout<<lightpath_database_split_greedy_result[i].lightpath_used_slots[j]<<" ";
			//}
			//cout<<endl<<"path: ";
			//for (int k = 0; k < lightpath_database_split_greedy_result[i].lightpath_path.size(); k++)
			//{
				//cout<<lightpath_database_split_greedy_result[i].lightpath_path[k]<<" ";
			//}
			//cout<<endl;
		//}
	//}
	for (int i = 0; i < TOPO_LINK_NUM; i++)
	{
		delete [] temp_optical_database_postsplit[i].spectrum_occupy;
	}
	delete [] temp_optical_database_postsplit;
	delete [] temp_lightpath_database_postsplit;
	delete [] temp_req_postsplit;
	delete [] lightpath_database_split_greedy_result;
	return;
}

float Network::Calculate_Throughput(int split_num, int *split_index, map<int, int> &split_lp, map<int, int> &split_point, Optical *optical_db, Lightpath *lp, Traffic *req, int split_option_B, float *affected_bw, float *average_hops, float *increased_hops_bw)
{
	Optical *temp_optical_database = new Optical[TOPO_LINK_NUM];
	Lightpath *temp_lightpath_database = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	Traffic *temp_req = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];
	Copy_Optical_Database(optical_db, temp_optical_database);
	Copy_Lightpath_Database(lp, temp_lightpath_database);
	Copy_Traffic_Database(req, temp_req);

	*affected_bw = 0;
	*increased_hops_bw = 0;

	Lightpath *temp_lightpath_splitresult = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	
	map<int, vector<int>> splitpoints; //record the index of splitpoints according to the index of lightpath
	for (int r = 0; r < split_num; r++)
	{
		splitpoints[split_lp[split_index[r]]].push_back(split_point[split_index[r]]);
	}

	for (int s = 0; s < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; s++)
	{
		if (splitpoints.count(s))
		{
			bubble(splitpoints[s], splitpoints[s].size());
			//it is possible that we have the different split points on the same lightpath
			Split_Lightpath(s, splitpoints[s], temp_optical_database, temp_lightpath_database, temp_lightpath_splitresult, split_option_B);
			*affected_bw += temp_lightpath_database[s].lightpath_supporting_bw;
			*increased_hops_bw += temp_lightpath_database[s].lightpath_supporting_bw * splitpoints[s].size();
		} 
		else
		{
			continue;
		}
	}
	OldNew_Lightpath_Combine(splitpoints, temp_lightpath_database, temp_lightpath_splitresult);
	Routing_and_Resource_Allocation(temp_optical_database, temp_lightpath_splitresult, temp_req, 1);

	float result_throughput = 0;
	int incre_access_no = 0;
	int total_hops = 0;
	for (int x = 0; x < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); x++)
	{
		result_throughput += temp_req[x].traffic_success*temp_req[x].traffic_actual_bw;
		incre_access_no += temp_req[x].traffic_success;
		total_hops += temp_req[x].traffic_hops*temp_req[x].traffic_actual_bw;
	}
	*average_hops = (float)total_hops/result_throughput;

	for (int y = 0; y < TOPO_LINK_NUM; y++)
	{
		delete [] temp_optical_database[y].spectrum_occupy;
	}
	delete [] temp_optical_database;
	delete [] temp_lightpath_database;
	delete [] temp_req;

	return result_throughput;
}

void Network::Splitpoint_Exchange(int splitpoint_num, int splitpoint_pool_size, int *old_splitpoints, int *new_splitpoints)
{
	srand( (unsigned)time(NULL) ); //seed for random
	int rand_old_index = rand() % splitpoint_num;
	int rand_new = rand() % splitpoint_pool_size;
	
	while (1)
	{
		int search_flag_2 = 0;
		for (int i = 0; i < splitpoint_num; i++)
		{
			if (old_splitpoints[i] == rand_new)
			{
				search_flag_2 = 1;
				break;
			}
		}

		if (search_flag_2 == 0) // no overlap
		{
			break;
		}
		else
		{
			rand_new = rand() % splitpoint_pool_size;
		}
	}

	//delete rand_old from olp_splitpoints, and add rand_new, so that we form new_splitpoints
	for (int s = 0; s < splitpoint_num; s++)
	{
		if (s == rand_old_index)
		{
			new_splitpoints[s] = rand_new;
		}
		else
		{
			new_splitpoints[s] = old_splitpoints[s];
		}
	}
}

void Network::SA_Lightpath_Splitting(int splitpoint_num, int split_option_B, Optical *o_db, Lightpath *lp_db, Traffic *req)
{
	srand( (unsigned)time(NULL) ); //seed for random
	double tet = (double) splitpoint_num/10;
	double SA_initial_temperature = exp(tet)*100;
	double SA_end_temperature = 0.01/(splitpoint_num*splitpoint_num*splitpoint_num);
	double SA_cooling_param = 0.95;

	lightpath_control = pre_split_lightpath_control;
	overall_traffic_success = pre_split_overall_traffic_success;

	Optical *optical_database_split_SA = new Optical[TOPO_LINK_NUM];
	Lightpath *lightpath_database_split_SA = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	Traffic *traffic_SA = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];
	
	Copy_Optical_Database(o_db, optical_database_split_SA);
	Copy_Lightpath_Database(lp_db, lightpath_database_split_SA);
	Copy_Traffic_Database(req, traffic_SA);

	float original_affected_bandwidth;
	float virtual_affected_bandwidth;
	float original_average_hops;
	float virtual_average_hops;
	float original_increased_hops;
	float virtual_increased_hops;
	//////////////////////////////////////////////////////////////////////////
	//all possible splitpoints
	map<int, int> possible_split_lightpath;
	map<int, int> possible_split_point;
	int nn = 0;
	for (int r = 0; r < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM; r++)
	{
		if (lightpath_database_split_SA[r].lightpath_path.size() > 2 && lightpath_database_split_SA[r].lightpath_modulation < 4) //lightpaths that can be split possibly
		{
			for (int w = 1; w < lightpath_database_split_SA[r].lightpath_path.size()-1; w++)
			{
				possible_split_lightpath[nn] = r;
				possible_split_point[nn] = w;
				nn++;
			}
		}
	}

	//initialize splitpoint_num splitpoints
	int splitpoints_pool_size = possible_split_lightpath.size();
	int *selected_splitpoints = new int[splitpoint_num]; //record index in terms of splitpoints_pool_size
	selected_splitpoints[0] = rand() % splitpoints_pool_size;
	for (int j = 1; j < splitpoint_num; j++)
	{
		while (1)
		{
			selected_splitpoints[j] = rand() % splitpoints_pool_size;
			int flag_2 = 0;
			for (int k = 0; k < j; k++)
			{
				if (selected_splitpoints[j] == selected_splitpoints[k])
				{
					flag_2 = 1;
				}
			}
			if (flag_2 == 0)
			{
				break;
			}
		}
	}
	//cout<<"Original splitpoints:"<<endl;
	//for (int w = 0; w < splitpoint_num; w++)
	//{
		//cout<<"--"<<endl;
		//cout<<"["<<possible_split_lightpath[selected_splitpoints[w]]<<"] "<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_src<<"->"<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_dst<<", modulation format: "<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_modulation<<", residual bandwidth: "<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_residual_bw<<", used spectrum: "<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_used_slots_num<<"number: ";
		//for (int j = 0; j < lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_used_slots.size(); j++)
		//{
			//cout<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_used_slots[j]<<" ";
		//}
		//cout<<endl<<"path: ";
		//for (int k = 0; k < lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_path.size(); k++)
		//{
			//cout<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_path[k]<<" ";
		//}
		//cout<<"Split Point: "<<lightpath_database_split_SA[possible_split_lightpath[selected_splitpoints[w]]].lightpath_path[possible_split_point[selected_splitpoints[w]]]<<endl;
	//}

	int temp_1 = lightpath_control;
	int temp_2 = overall_traffic_success;
	split_lightpath_control = 0;
	float original_throughput = Calculate_Throughput(splitpoint_num, selected_splitpoints, possible_split_lightpath, possible_split_point, optical_database_split_SA, lightpath_database_split_SA, traffic_SA, split_option_B, &original_affected_bandwidth, &original_average_hops, &original_increased_hops);
	lightpath_control = temp_1;
	overall_traffic_success = temp_2;

	//////////////////////////////////////////////////////////////////////////
	//Simulated Annealing
	float temperature = SA_initial_temperature;
	while (temperature > SA_end_temperature)
	{
		//cout<<"original: "<<original_throughput;
		//cout<<"---"<<endl;
		int *virtual_selected_points = new int[splitpoint_num];
		Splitpoint_Exchange(splitpoint_num, splitpoints_pool_size, selected_splitpoints, virtual_selected_points);
		//cout<<"Current splitpoints:"<<endl;
		//for (int w = 0; w < splitpoint_num; w++)
		//{
			//cout<<"--"<<endl;
			//cout<<"["<<possible_split_lightpath[virtual_selected_points[w]]<<"] "<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_src<<"->"<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_dst<<", modulation format: "<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_modulation<<", residual bandwidth: "<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_residual_bw<<", used spectrum: "<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_used_slots_num<<"number: ";
			//for (int j = 0; j < lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_used_slots.size(); j++)
			//{
				//cout<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_used_slots[j]<<" ";
			//}
			//cout<<endl<<"path: ";
			//for (int k = 0; k < lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_path.size(); k++)
			//{
				//cout<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_path[k]<<" ";
			//}
			//cout<<"Split Point: "<<lightpath_database_split_SA[possible_split_lightpath[virtual_selected_points[w]]].lightpath_path[possible_split_point[virtual_selected_points[w]]]<<endl;
		//}

		int temp_4 = lightpath_control;
		int temp_5 = overall_traffic_success;
		split_lightpath_control = 0;
		float virtual_throughput = Calculate_Throughput(splitpoint_num, virtual_selected_points, possible_split_lightpath, possible_split_point, optical_database_split_SA, lightpath_database_split_SA, traffic_SA, split_option_B, &virtual_affected_bandwidth, &virtual_average_hops, &virtual_increased_hops);
		lightpath_control = temp_4;
		overall_traffic_success = temp_5;
		//cout<<", Virtual thrpt: "<<virtual_throughput<<" (";

		double eta;
		if (virtual_throughput >= original_throughput)
		{
			eta = 1;
		}
		else
		{
			eta = exp((virtual_throughput-original_throughput)/temperature);
		}

		double ruler;
		int temp = rand() % 100;
		ruler = (double) temp/100;

		//cout<<"Temperature: "<<temperature<<", ";
		if (eta > ruler)
		{
			//cout<<eta<<" > "<<ruler<<", accepted, ";
			for (int s = 0; s < splitpoint_num; s++)
			{
				selected_splitpoints[s] = virtual_selected_points[s];
			}
			original_throughput = virtual_throughput;
			original_affected_bandwidth = virtual_affected_bandwidth;
			original_average_hops = virtual_average_hops;
			original_increased_hops = virtual_increased_hops;
		}
		else
		{
			//cout<<eta<<" <= "<<ruler<<", declined, ";
		}
		//cout<<"Throughput: "<<original_throughput<<")"<<endl;
		temperature = temperature*SA_cooling_param;
		delete [] virtual_selected_points;
	}

	//cout<<"current Lightpath number: "<<lightpath_control<<endl;
	//cout<<"*SA throughput:"<<original_throughput<<" (*6.25)Gbps"<<endl;
	//cout<<"*Affected bandwidth: "<<original_affected_bandwidth<<endl;
	//cout<<"*average bps traffic hops: "<<original_average_hops<<endl<<endl;
	//cout<<"*throughput, Affected bandwidth, average bps traffic hops"<<endl;
	cout<<original_throughput<<" "<<original_affected_bandwidth<<" "<<original_average_hops<<" "<<original_increased_hops<<endl;

	for (int i = 0; i < TOPO_LINK_NUM; i++)
	{
		delete [] optical_database_split_SA[i].spectrum_occupy;
	}
	delete [] optical_database_split_SA;
	delete [] lightpath_database_split_SA;
	delete [] traffic_SA;
	delete [] selected_splitpoints;
	return;
}

void Network::All_Lightpath_Splitting(int split_option_B, Optical *o_db, Lightpath *lp_db, Traffic *req)
{
	lightpath_control = pre_split_lightpath_control;
	overall_traffic_success = pre_split_overall_traffic_success;
	split_lightpath_control = 0;

	Optical *temp_optical_database = new Optical[TOPO_LINK_NUM];
	Lightpath *temp_lightpath_database = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];
	Traffic *temp_req = new Traffic[TOPO_NODE_NUM*(TOPO_NODE_NUM-1)];	
	Copy_Optical_Database(o_db, temp_optical_database);	
	Copy_Lightpath_Database(lp_db, temp_lightpath_database);
	Copy_Traffic_Database(req, temp_req);
	Lightpath *temp_lightpath_splitresult = new Lightpath[TOPO_LINK_NUM*SPECTRUM_SLOT_NUM];

	float affected_bandwidth = 0;
	int splitpoint_num = 0;
	float increased_hops_bw = 0;

	map<int, vector<int>> split_lp_and_points;

	for (int y = 0; y < lightpath_control; y++)
	{
		if (temp_lightpath_database[y].lightpath_path.size() > 2 && temp_lightpath_database[y].lightpath_modulation < 4)
		{
			vector<int> current_splitpoints;
			for (int z = 1; z < temp_lightpath_database[y].lightpath_path.size()-1; z++)
			{
				current_splitpoints.push_back(z);
				split_lp_and_points[y].push_back(z);
			}
			Split_Lightpath(y, current_splitpoints, temp_optical_database, temp_lightpath_database, temp_lightpath_splitresult, split_option_B);
			affected_bandwidth += temp_lightpath_database[y].lightpath_supporting_bw;
			splitpoint_num += temp_lightpath_database[y].lightpath_path.size() - 2;
			increased_hops_bw += temp_lightpath_database[y].lightpath_supporting_bw * (temp_lightpath_database[y].lightpath_path.size() - 2);
		}
	}

	OldNew_Lightpath_Combine(split_lp_and_points, temp_lightpath_database, temp_lightpath_splitresult);
	Routing_and_Resource_Allocation(temp_optical_database, temp_lightpath_splitresult, temp_req, 1);
	float incremental_postsplit_throughput = 0;
	int incre_postsplit_access_no = 0;
	int total_hops = 0;
	float average_hops;

	for (int x = 0; x < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); x++)
	{
		incremental_postsplit_throughput += temp_req[x].traffic_success*temp_req[x].traffic_actual_bw;
		incre_postsplit_access_no += temp_req[x].traffic_success;
		total_hops += temp_req[x].traffic_hops*temp_req[x].traffic_actual_bw;
	}
	average_hops = (float)total_hops/incremental_postsplit_throughput;
	cout<<incremental_postsplit_throughput<<" "<<affected_bandwidth<<" "<<average_hops<<" "<<increased_hops_bw<<endl;

	for (int i = 0; i < TOPO_LINK_NUM; i++)
	{
		delete [] temp_optical_database[i].spectrum_occupy;
	}
	delete [] temp_optical_database;
	delete [] temp_lightpath_database;
	delete [] temp_lightpath_splitresult;
	delete [] temp_req;
}

void Network::Lightpath_Splitting_Routing()
{
	clock_t startTime, endTime;
	//Lightpath Splitting
	for (int k = 1; k < 16; k++)
	{
		cout<<"*******"<<endl<<"K="<<10*k<<endl<<"*******"<<endl<<endl;
		//////////////////////////////////////////////////////////////////////////
		//Select as many lightpaths as possible, split to max electric capacity, BF-MaxE
		cout<<"***[Greedy algorithm 1,2,3,4,Simulated annealing algorithm 1,2]***"<<endl;
		cout<<"*throughput(*6.25Gbps), Affected bandwidth(*6.25Gbps), LPSplitting accommodateting average bps traffic hops(/6.25Gbps), LPSplitting increased hops to baseline traffic*baseline bandwidth"<<endl;
		startTime = clock();
		Greedy_Lightpath_Splitting(10*k, 0, 0, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"BF-MaxE: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

		//////////////////////////////////////////////////////////////////////////
		//split one lightpath as much as possible, split to max electric capacity, DF-MaxE
		//cout<<"***[Greedy algorithm 2]***"<<endl;
		startTime = clock();
		Greedy_Lightpath_Splitting(10*k, 1, 0, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"DF-MaxE: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

		//////////////////////////////////////////////////////////////////////////
		//Select as many lightpaths as possible, split to max optical capacity, BF-MaxO
		//cout<<"***[Greedy algorithm 3]***"<<endl;
		startTime = clock();
		Greedy_Lightpath_Splitting(10*k, 0, 1, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"BF-MaxO: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

		//////////////////////////////////////////////////////////////////////////
		//split one lightpath as much as possible, split to max optical capacity, DF-MaxO
		//cout<<"***[Greedy algorithm 4]***"<<endl;
		startTime = clock();
		Greedy_Lightpath_Splitting(10*k, 1, 1, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"DF-MaxO: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

		//////////////////////////////////////////////////////////////////////////
		//Simulated annealing, split to max electric capacity, SA-MaxE
		//cout<<"***[Simulated annealing algorithm 1]***"<<endl;
		startTime = clock();
		SA_Lightpath_Splitting(10*k, 0, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"SA-MaxE: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

		//////////////////////////////////////////////////////////////////////////
		//Simulated annealing, split to max optical capacity, SA-MaxO
		//cout<<"***[Simulated annealing algorithm 2]***"<<endl;
		startTime = clock();
		SA_Lightpath_Splitting(10*k, 1, optical_database, lightpath_database, incremental_residual_traffic);
		endTime = clock();
		cout<<"SA-MaxO: "<<(double)(endTime - startTime)/CLOCKS_PER_SEC<<"s"<<endl;

	}
	cout<<"***[all splitting algorithm]***"<<endl;
	All_Lightpath_Splitting(0, optical_database, lightpath_database, incremental_residual_traffic);
	All_Lightpath_Splitting(1, optical_database, lightpath_database, incremental_residual_traffic);

	return;
}

void Network::Post_Processing()
{
	//Optical
	for (int i = 0; i < TOPO_LINK_NUM; i++)
	{
		delete [] optical_database[i].spectrum_occupy;
	}
	delete [] optical_database;

	//Traffic
	delete [] baseline_traffic;
	delete [] incremental_traffic;
	delete [] incremental_residual_traffic;

	//Generate_Baseline_Configurations()
	delete [] lightpath_database;
		
	exit(0);
}

int main()
{
	cout<<"Welcome to Lightpath Splitting Simulation Environment"<<endl<<"Copyright z.zhong.tsinghua@gmail.com"<<endl<<"2017.11 --version 1.0 @Tsinghua"<<endl<<endl;
	while(1)
	{
		while(1)
		{
			cout<<"========================================================="<<endl<<"========================================================="<<endl;
			cout<<"1.Read Topology File"<<endl<<"2.Generate Traffic Profile"<<endl<<"3.Generate Baseline Configurations"<<endl<<"4.Conventional Incremental Routing"<<endl<<"5.Lightpath Splitting Routing"<<endl<<"6.Exit"<<endl;
			char main_window_choice;
			cin>>main_window_choice;	

			switch (main_window_choice)
			{
			case '1' : network.Read_Topo(); break;
			case '2' : network.Generate_Traffic(MU); break;
			case '3' : network.Generate_Baseline_Configurations(); break;
			case '4' : network.General_Incremental_Routing();break;
			case '5' : network.Lightpath_Splitting_Routing();break; 
			case '6' : network.Post_Processing();
			default : cout<<"Wrong Input!";continue;
			}
		}
	}
	return 0;
}