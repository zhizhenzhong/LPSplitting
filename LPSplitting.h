#ifndef LPSPLITTING_H
#define LPSPLITTING_H

#define SLOT_CAPACITY 2 //(means 12.5Gb/s)
#define BANDWIDTH_RANGE 12 //(0 ~ 20, 5G ~ 50G)
#define MU 2

#define ELECTRIC_LINK_WEIGHT 1
#define OPTICAL_LINK_WEIGHT 100
#define TRANSPONDER_LINK_WEIGHT 100
#define SPLITTING_WEIGHT 10000

//USNet topo
//#define TOPO_NODE_NUM 24
//#define TOPO_LINK_NUM 86
//#define SPECTRUM_SLOT_NUM 48

//6 mode topo
//#define TOPO_NODE_NUM 6
//#define TOPO_LINK_NUM 18
//#define SPECTRUM_SLOT_NUM 10

//ChinaNet topo
//#define TOPO_NODE_NUM 27
//#define TOPO_LINK_NUM 76
//#define SPECTRUM_SLOT_NUM 70

//NSFNET topo
#define TOPO_NODE_NUM 14
#define TOPO_LINK_NUM 40
#define SPECTRUM_SLOT_NUM 30

#define BPSK 9600
#define QPSK 4800
#define QAM_8 2400
#define QAM_16 1200

#define MAX 999999999

#include <iostream>
#include <vector>
#include <map>
#include <stack>

using namespace std;

class Node
{
public:
	int occupy_rx_oe;
	int occupy_tx_eo;
	Node();
	~Node(){};
};

class Optical : public Node
{
public:
	int optical_src;
	int optical_dst;
	int *spectrum_occupy;
	int optical_link_capacity;
	int optical_length;
	
	Optical();
	~Optical(){};
};

class Lightpath : public Optical
{
public:
	int lightpath_src;
	int lightpath_dst;
	int lightpath_modulation;
	float lightpath_residual_bw;
	float lightpath_supporting_bw;
	int lightpath_used_slots_num;
	vector<int> lightpath_used_slots;
	vector<int> lightpath_path;
	
	Lightpath();
	~Lightpath(){};
};

class Traffic : public Lightpath
{
public:
	int traffic_ID;
	int traffic_src;
	int traffic_dst;
	float traffic_bw;
	float traffic_actual_bw;
	int traffic_success;
	int traffic_hops;

	Traffic();
	~Traffic(){};
};

class Network : public Traffic
{
public:
	Optical *optical_database;
	Lightpath *lightpath_database;

	Traffic *baseline_traffic;
	Traffic *incremental_traffic;
	Traffic *incremental_residual_traffic;
	
	map<int, int> traffic_latency;
	map<int, float> traffic_link_load;
	map<int, float> network_load;
	map<int, int> lightpath_hop_product;
	map<int, float> lightpath_average_hop;

	vector<int> ele_path_built;  //electrical path in electrical database
	vector<int> path_built;      //optical path in optical database

	int lightpath_number;  //current number of lightpaths

	Network();
	void Read_Topo();
	void Generate_Traffic(float); 
	void Combine(int, int, int*, int*, const int, map<int, vector<int>>&);
	int Lightpath_Modulation(int, int);

	void Construnt_Current_Topo(Optical*, Lightpath*, int**, float, int);
	int Path_Resource_Allocation(Optical*, Lightpath*, Traffic*, int, vector<int>, float, int);
	void Routing_and_Resource_Allocation(Optical*, Lightpath*, Traffic*, int);

	void Greedy_Target_LP_Selection(int, int, map<int, int>&, map<int, int>&, Traffic*, Lightpath*, Optical*);
	vector<int> Greedy_Splitpoint_Determination(int, Lightpath*, int, int);
	int Split_Lightpath(int, vector<int>, Optical*, Lightpath*, Lightpath*, int);
	float Calculate_Throughput(int, int*, map<int, int>&, map<int, int>&, Optical*, Lightpath*, Traffic*, int, float*, float*, float*);
	void Splitpoint_Exchange(int, int, int*, int*);
	void OldNew_Lightpath_Combine(map<int, vector<int>>&, Lightpath*, Lightpath*);

	void Greedy_Lightpath_Splitting(int, int, int, Optical*, Lightpath*, Traffic*);
	void SA_Lightpath_Splitting(int, int, Optical*, Lightpath*, Traffic*);
	void All_Lightpath_Splitting(int, Optical*, Lightpath*, Traffic*);

	void Generate_Baseline_Configurations();
	void General_Incremental_Routing();
	
	void Lightpath_Splitting_Routing();
	void Post_Processing();

	~Network(){};
};

void Copy_Optical_Database(Optical *original, Optical *newone)
{
	for (int x = 0; x < TOPO_LINK_NUM; x++)
	{
		newone[x].optical_src = original[x].optical_src;
		newone[x].optical_dst = original[x].optical_dst;
		newone[x].optical_link_capacity = original[x].optical_link_capacity;
		newone[x].optical_length = original[x].optical_length;
		newone[x].spectrum_occupy = new int[SPECTRUM_SLOT_NUM];
		for (int y = 0; y < SPECTRUM_SLOT_NUM; y++)
		{
			newone[x].spectrum_occupy[y] = original[x].spectrum_occupy[y];
		}
	}
}

void Copy_Lightpath_Database(Lightpath *original, Lightpath *newone)
{
	for (int uu = 0; uu < TOPO_LINK_NUM*SPECTRUM_SLOT_NUM*0.5; uu++)
	{
		newone[uu].lightpath_src = original[uu].lightpath_src;
		newone[uu].lightpath_dst = original[uu].lightpath_dst;
		newone[uu].lightpath_modulation = original[uu].lightpath_modulation;
		newone[uu].lightpath_residual_bw = original[uu].lightpath_residual_bw;
		newone[uu].lightpath_supporting_bw = original[uu].lightpath_supporting_bw;
		newone[uu].lightpath_used_slots_num = original[uu].lightpath_used_slots_num;
		newone[uu].lightpath_used_slots.clear();
		for (int r = 0; r < original[uu].lightpath_used_slots.size(); r++)
		{
			newone[uu].lightpath_used_slots.push_back(original[uu].lightpath_used_slots[r]);
		}
		newone[uu].lightpath_path.clear();
		for (int e = 0; e < original[uu].lightpath_path.size(); e++)
		{
			newone[uu].lightpath_path.push_back(original[uu].lightpath_path[e]);
		}
	}
}

void Copy_Traffic_Database(Traffic *original, Traffic *newone)
{
	for (int i = 0; i < TOPO_NODE_NUM*(TOPO_NODE_NUM-1); i++)
	{
		newone[i].traffic_ID = original[i].traffic_ID;
		newone[i].traffic_src = original[i].traffic_src;
		newone[i].traffic_dst = original[i].traffic_dst;
		newone[i].traffic_bw = original[i].traffic_bw;
		newone[i].traffic_actual_bw = original[i].traffic_actual_bw;
		newone[i].traffic_success = original[i].traffic_success;
	}
}

//bubble sorting
void bubble_sort(Traffic *req, int n)
{
	Traffic temp;

	for (int j = 0; j < n-1; j++)
	{
		for (int i = 0; i < n-1-j; i++)
		{
			if (req[i].traffic_bw < req[i+1].traffic_bw)
			{
				temp.traffic_ID = req[i].traffic_ID;
				temp.traffic_src = req[i].traffic_src;
				temp.traffic_dst = req[i].traffic_dst;
				temp.traffic_bw = req[i].traffic_bw;
				temp.traffic_actual_bw = req[i].traffic_actual_bw;
				temp.traffic_success = req[i].traffic_success;
				
				req[i].traffic_ID = req[i+1].traffic_ID;
				req[i].traffic_src = req[i+1].traffic_src;
				req[i].traffic_dst = req[i+1].traffic_dst;
				req[i].traffic_bw = req[i+1].traffic_bw;
				req[i].traffic_actual_bw = req[i+1].traffic_actual_bw;
				req[i].traffic_success = req[i+1].traffic_success;
				
				req[i+1].traffic_ID = temp.traffic_ID;
				req[i+1].traffic_src = temp.traffic_src;
				req[i+1].traffic_dst = temp.traffic_dst;
				req[i+1].traffic_bw = temp.traffic_bw;
				req[i+1].traffic_actual_bw = temp.traffic_actual_bw;
				req[i+1].traffic_success = temp.traffic_success;
			}
		}
	}
}

void bubble_sort_lp(Lightpath *lp, map<int, int> &old_index, int n)
{
	Lightpath temp;
	int tt;
	for (int j = 0; j < n-1; j++)
	{
		for (int i = 0; i < n-1-j; i++)
		{
			if (lp[i].lightpath_supporting_bw < lp[i+1].lightpath_supporting_bw)
			{
				temp.lightpath_src = lp[i].lightpath_src;
				temp.lightpath_dst = lp[i].lightpath_dst;
				temp.lightpath_modulation = lp[i].lightpath_modulation;
				temp.lightpath_residual_bw = lp[i].lightpath_residual_bw;
				temp.lightpath_supporting_bw = lp[i].lightpath_supporting_bw;
				temp.lightpath_used_slots_num = lp[i].lightpath_used_slots_num;
				temp.lightpath_used_slots.clear();
				for (int s = 0; s < lp[i].lightpath_used_slots.size(); s++)
				{
					temp.lightpath_used_slots.push_back(lp[i].lightpath_used_slots[s]);
				}
				temp.lightpath_path.clear();
				for (int t = 0; t < lp[i].lightpath_path.size(); t++)
				{
					temp.lightpath_path.push_back(lp[i].lightpath_path[t]);
				}
				tt = old_index[i];

				lp[i].lightpath_src = lp[i+1].lightpath_src;
				lp[i].lightpath_dst = lp[i+1].lightpath_dst;
				lp[i].lightpath_modulation = lp[i+1].lightpath_modulation;
				lp[i].lightpath_residual_bw = lp[i+1].lightpath_residual_bw;
				lp[i].lightpath_supporting_bw = lp[i+1].lightpath_supporting_bw;
				lp[i].lightpath_used_slots_num = lp[i+1].lightpath_used_slots_num;
				lp[i].lightpath_used_slots.clear();
				for (int s = 0; s < lp[i+1].lightpath_used_slots.size(); s++)
				{
					lp[i].lightpath_used_slots.push_back(lp[i+1].lightpath_used_slots[s]);
				}
				lp[i].lightpath_path.clear();
				for (int t = 0; t < lp[i+1].lightpath_path.size(); t++)
				{
					lp[i].lightpath_path.push_back(lp[i+1].lightpath_path[t]);
				}
				old_index[i] = old_index[i+1];

				lp[i+1].lightpath_src = temp.lightpath_src;
				lp[i+1].lightpath_dst = temp.lightpath_dst;
				lp[i+1].lightpath_modulation = temp.lightpath_modulation;
				lp[i+1].lightpath_residual_bw = temp.lightpath_residual_bw;
				lp[i+1].lightpath_supporting_bw = temp.lightpath_supporting_bw;
				lp[i+1].lightpath_used_slots_num = temp.lightpath_used_slots_num;
				lp[i+1].lightpath_used_slots.clear();
				for (int s = 0; s < temp.lightpath_used_slots.size(); s++)
				{
					lp[i+1].lightpath_used_slots.push_back(temp.lightpath_used_slots[s]);
				}
				lp[i+1].lightpath_path.clear();
				for (int t = 0; t < temp.lightpath_path.size(); t++)
				{
					lp[i+1].lightpath_path.push_back(temp.lightpath_path[t]);
				}
				old_index[i+1] = tt;
			}
		}
	}
}

void bubble(vector<int> &seq, int n)
{
	int temp;

	for (int j = 0; j < n-1; j++)
	{
		for (int i = 0; i < n-1-j; i++)
		{
			if (seq[i] > seq[i+1])
			{
				temp = seq[i];
				seq[i] = seq[i+1];
				seq[i+1] = temp;
			}
		}
	}
}

vector<int> dijkstra(int **g, int src, int dst, int size_number)
{
	int *dist = new int[size_number];
	int *prev = new int[size_number];
	vector<int> unvisited;

	for (int i = 0; i < size_number; i++)
	{
		prev[i] = -1;
		if (i == src)
		{
			dist[i] = 0;
		}
		else
		{
			dist[i] = MAX;

		}
		unvisited.push_back(i);
	}

	int temp_flag = 0;

	while (unvisited.size() != 0 && temp_flag == 0)
	{
		int u = unvisited[0];
		for (int x = 0; x < unvisited.size(); x++)
		{
			if (dist[unvisited[x]] < dist[u] && dist[unvisited[x]] < MAX)
			{
				u = unvisited[x];

				if (u == dst)
				{
					temp_flag = 1;
					break;
				}
			}
		}

		vector<int>::iterator it;
		for(it = unvisited.begin(); it != unvisited.end();)
		{
			if(*it == u)
				it = unvisited.erase(it); 
			else
				++it;
		}

		for (int k = 0; k < size_number; k++)
		{
			if (g[u][k] < MAX)
			{
				int current_length = g[u][k] + dist[u];
				if (current_length < dist[k])
				{
					dist[k] = current_length;
					prev[k] = u;
				}
			}
		}
	}

	vector<int> path;
	int t = dst;

	while (prev[t] > -1)
	{
		path.push_back(t);
		int x = prev[t];
		t = x;
	}
	path.push_back(t);

	if (path.size() == 1)
	{
		path.clear();
	}

	vector<int> reverse_result;
	for (int y = 0; y < path.size(); y++)
	{
		reverse_result.push_back(path[path.size()-y-1]);
	}

	delete [] dist;
	delete [] prev;

	return reverse_result;
}

#endif