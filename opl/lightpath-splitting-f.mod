/*********************************************
 * OPL 12.6.1.0 Model
 * Author: Zhizhen Zhong
 * Creation Date: 2017-5-29
 *********************************************/

 //Nodes
 {int} Nodes = ...;
 
 //Fiber links
 tuple link
 {
 	int src_link;
 	int dst_link;
 }
 {link} Links = ...;
 int Dis[Links] = ...;
 
 {int} Bandwidth = ...;
 
 //Lightpaths
 tuple lightpath
 {
 	int src_lightpath;
 	int dst_lightpath;
 	int flag; //whether this lightpath is baseline, 0 baseline cannot degrade, 1 incremental
 }
 {lightpath} Lightpaths = ...;
 
 tuple new_lightpath
 {
 	int src_new_lightpath;
 	int dst_new_lightpath;
 	int new_flag; //whether this lightpath is baseline, 0 baseline cannot degrade, 1 incremental
 }
 {new_lightpath} New_Lightpaths = ...;
 
 tuple split_lp
 {
 	int src_split_lp;
 	int dst_split_lp; 
 }
 {split_lp} Split_Lps = ...;
  
 //Modulation
 {int} Modulations = ...;
 int Reach[Modulations] = ...;
 
 //Requests
 tuple request
 {
 	 int src_request;
 	 int dst_request;
 	 int bandwidth;
 	 int ddl;
 	 int priority;
 }
 {request} Requests = ...;
 
 tuple new_request
 {
 	int src_new_request;
 	int dst_new_request;
 	int bw_new_request; 
 }
 {new_request} New_Requests = ...;
 
 /////////////////////////////////////Parameters
 int slot_number = ...;
 int maximum_number = ...;
 int spectrum_size = ...;
 int times = ...;
 
 //Baseline results
 {link} O_Path[Lightpaths] = ...;
 {split_lp} O_Split_Lp[Lightpaths] = ...;
 int Occupied_Lightpath_Capacity[Lightpaths] = ...;
 int Mod_lightpath[Lightpaths] = ...;
 int Width_lightpath[Lightpaths] = ...;
 int Hops_lightpath[Lightpaths] = ...;
  
 ////////////////////////////////////Variables
 //variables for baseline lightpath splitting
 dvar int pai[<i,j,0> in Lightpaths][<x,y> in Split_Lps] in 0..1;
 dvar int lambda_split[f in 1..slot_number][<i,j,0> in Lightpaths][<x,y> in Split_Lps][<m,n> in Links] in 0..1;
 dvar int X_split[f in 1..slot_number][<i,j,0> in Lightpaths][<x,y> in Split_Lps] in 0..1;
 dvar int theta_split[<i,j,0> in Lightpaths][<x,y> in Split_Lps][a in Modulations] in 0..1;
 
 //variables for new lightpaths
 dvar int lambda[f in 1..slot_number][<i,j,1> in New_Lightpaths][<m,n> in Links] in 0..1;
 dvar int X[f in 1..slot_number][<i,j,1> in New_Lightpaths] in 0..1;
 dvar int theta_incre[<i,j,1> in New_Lightpaths][a in Modulations] in 0..1;
 dvar int alpha_2[<ns,nd,nbw> in New_Requests][<i,j,1> in New_Lightpaths] in 0..1;
 dvar int e_incre[<ns,nd,nbw> in New_Requests] in 0..1;
 dvar int oo[<i,j,1> in New_Lightpaths][<m,n> in Links] in 0..1;
 
 dvar int rho[<i,j,1> in New_Lightpaths][f in 1..slot_number][a in Modulations] in 0..1;
 dvar int rho_split[<i,j,0> in Lightpaths][<x,y> in Split_Lps][f in 1..slot_number][a in Modulations] in 0..1;
 dvar int rate[band in Bandwidth][<ns,nd,nbw> in New_Requests] in 0..1;
 dvar int per[band in Bandwidth][<ns,nd,nbw> in New_Requests][<i,j,1> in New_Lightpaths] in 0..1;
 
 dvar int degradation_times;
 dvar int throughput;

 maximize throughput*100 - degradation_times;

 subject to
 {
    degradation_times == sum(<i,j,0> in Lightpaths, <x,y> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<x,y>] - 30; 
    throughput == sum(r in New_Requests, band in Bandwidth)band*rate[band][r]*times;
       
    /////////////////////////////////////////////////////
    //EXISTING LIGHTPATH DEGRADATION
    //Eq(19), existing lightpath splitting routing  	
 	forall(<i,j,0> in Lightpaths, x in Nodes)
 	  if(x == i)
 	  	sum(y in Nodes: <x,y> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<x,y>] - sum(y in Nodes: <y,x> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<y,x>] == 1;
 	  else if(x == j)
 	  	sum(y in Nodes: <x,y> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<x,y>] - sum(y in Nodes: <y,x> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<y,x>] == -1;
 	  else
 	  	sum(y in Nodes: <x,y> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<x,y>] - sum(y in Nodes: <y,x> in O_Split_Lp[<i,j,0>])pai[<i,j,0>][<y,x>] == 0;
    
    //Eq(20\21\22\23)
    forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps diff O_Split_Lp[<i,j,0>])
    {
      pai[<i,j,0>][<x,y>] == 0;
      sum(f in 1..slot_number, <m,n> in Links)lambda_split[f][<i,j,0>][<x,y>][<m,n>] == 0;
      sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a] == 0;
      sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] == 0;
    }
    
    //Eq(24)
    forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps, f in 1..slot_number, <m,n> in Links diff O_Path[<i,j,0>])
 	  lambda_split[f][<i,j,0>][<x,y>][<m,n>] == 0;
 	
 	//Eq(25)
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps, f in 1..slot_number, m in Nodes)
 	  if(m == x)
 	  	sum(n in Nodes:<m,n> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<m,n>] - sum(n in Nodes:<n,m> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<n,m>] == X_split[f][<i,j,0>][<x,y>];
 	  else if(m == y)
 	    sum(n in Nodes:<m,n> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<m,n>] - sum(n in Nodes:<n,m> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<n,m>] == -X_split[f][<i,j,0>][<x,y>];
 	  else
 	    sum(n in Nodes:<m,n> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<m,n>] - sum(n in Nodes:<n,m> in O_Path[<i,j,0>])lambda_split[f][<i,j,0>][<x,y>][<n,m>] == 0;
 	  
    //Eq(26), splitted lightpaths <= existing lightpaths hops
 	forall(<i,j,0> in Lightpaths)
 	{
 	  sum(<x,y> in Split_Lps)pai[<i,j,0>][<x,y>] <= Hops_lightpath[<i,j,0>];
 	  sum(<x,y> in Split_Lps)pai[<i,j,0>][<x,y>] >= 1;
 	}
 	
 	//eq(27),assign splitted lightpaths to optical spectrum slots  
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	{
 	  sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] <= maximum_number*pai[<i,j,0>][<x,y>];
 	  sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] >= pai[<i,j,0>][<x,y>];  
 	}
 	 	
 	//Eq(28)    
 	forall(<i,j,z> in Lightpaths, <x,y> in Split_Lps, f in 1..slot_number-1)
 	  -maximum_number*(X_split[f][<i,j,0>][<x,y>]-X_split[f+1][<i,j,0>][<x,y>] - 1) >= sum(f2 in f+2..slot_number)X_split[f2][<i,j,0>][<x,y>];
    
    //Eq(29), lightpath degradation, spectrum allocation 	
  	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
  	  sum(f in 1..slot_number, a in Modulations) a*rho_split[<i,j,0>][<x,y>][f][a] >= Width_lightpath[<i,j,0>]*Mod_lightpath[<i,j,0>]*pai[<i,j,0>][<x,y>]; 	   	  
 	
 	//Eq(30)	
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	  sum(a in Modulations)a*theta_split[<i,j,0>][<x,y>][a] >= Mod_lightpath[<i,j,0>]*pai[<i,j,0>][<x,y>];
 	
 	//Eq(31)  
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	  sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] <= Width_lightpath[<i,j,0>];
 	
 	//Eq(32)  
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	  sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a] <= 1;
 	  
 	//eq(33), 1109 add
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	{
 	  sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a] <= maximum_number*pai[<i,j,0>][<x,y>];
 	  sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a] >= pai[<i,j,0>][<x,y>];
 	}
 	
 	//Eq(34)  
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps,  f in 1..slot_number,a in Modulations)
 	  sum(<m,n> in Links)(lambda_split[f][<i,j,0>][<x,y>][<m,n>]*Dis[<m,n>]) <= Reach[a] - maximum_number*(theta_split[<i,j,0>][<x,y>][a]-1);
 	
 	//Eq(35)
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps)
 	{
 	  sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] <= maximum_number*sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a];
 	  sum(f in 1..slot_number)X_split[f][<i,j,0>][<x,y>] >= sum(a in Modulations)theta_split[<i,j,0>][<x,y>][a];
 	}
 	
 	///////////// Newly-Setup lightpaths
 	//Eq(36), spectrum slot used once, by incremental lightpaths or exising lightapths
 	forall(f in 1..slot_number, <m,n> in Links)
 	  sum(<i,j,0> in Lightpaths, <x,y> in Split_Lps)(lambda_split[f][<i,j,0>][<x,y>][<m,n>]) + sum(<i,j,1> in New_Lightpaths)lambda[f][<i,j,1>][<m,n>] <= 1;	 	
 	
 	//eq2, spectrum consecutive, incremental lightpaths
 	forall(<i,j,1> in New_Lightpaths, f in 1..slot_number-1, <m,n> in Links)
 	  -maximum_number*(X[f][<i,j,1>]-X[f+1][<i,j,1>] - 1) >= sum(f2 in f+2..slot_number)X[f2][<i,j,1>];
 	  
 	//eq3, optical flow conservation, incremental lightpaths
 	forall(<i,j,1> in New_Lightpaths, f in 1..slot_number, m in Nodes)
 	  if(m == i)
 	  	sum(n in Nodes:<m,n> in Links)lambda[f][<i,j,1>][<m,n>] - sum(n in Nodes:<n,m> in Links)lambda[f][<i,j,1>][<n,m>] == X[f][<i,j,1>];
 	  else if(m == j)
 	    sum(n in Nodes:<m,n> in Links)lambda[f][<i,j,1>][<m,n>] - sum(n in Nodes:<n,m> in Links)lambda[f][<i,j,1>][<n,m>] == -X[f][<i,j,1>];
 	  else
 	    sum(n in Nodes:<m,n> in Links)lambda[f][<i,j,1>][<m,n>] - sum(n in Nodes:<n,m> in Links)lambda[f][<i,j,1>][<n,m>] == 0;
 	    
 	//eq4, spectrum slot used once, incremental lightpaths
 	//forall(f in 1..slot_number, <m,n> in Links)
 	  //sum(<i,j,1> in New_Lightpaths)lambda[f][<i,j,1>][<m,n>] <= 1;
 	
 	//eq5
 	forall(<i,j,1> in New_Lightpaths, <m,n> in Links)
 	{
 	  sum(f in 1..slot_number)lambda[f][<i,j,1>][<m,n>] <= oo[<i,j,1>][<m,n>]*maximum_number;
 	  sum(f in 1..slot_number)lambda[f][<i,j,1>][<m,n>] >= oo[<i,j,1>][<m,n>];
 	}
 	   	
 	//eq6, cancel loops
 	forall(<i,j,1> in New_Lightpaths, m in Nodes)
 	  sum(n in Nodes: <m,n> in Links)oo[<i,j,1>][<m,n>] <= 1;
 	
 	//eq7
 	forall(<i,j,1> in New_Lightpaths, n in Nodes)
 	  sum(m in Nodes: <m,n> in Links)oo[<i,j,1>][<m,n>] <= 1;
 	
 	//eq8  
 	forall(<i,j,1> in New_Lightpaths, <m,n> in Links)
 	  oo[<i,j,1>][<m,n>] + oo[<i,j,1>][<n,m>] <= 1;
 	
 	//eq9, each lightpath has one modulation level, all lightpaths 	
 	forall(<i,j,1> in New_Lightpaths)
 	  sum(a in Modulations)theta_incre[<i,j,1>][a] <= 1;
 	
 	//eq10, modulation level transmission reach constraint, existing and new lightpaths
 	forall(<i,j,1> in New_Lightpaths, f in 1..slot_number, a in Modulations)
 	  sum(<m,n> in Links)(lambda[f][<i,j,1>][<m,n>]*Dis[<m,n>]) <= Reach[a] - maximum_number*(theta_incre[<i,j,1>][a]-1);
 	
 	//Eq(11)
 	forall(<i,j,1> in New_Lightpaths)
 	{
 	  sum(f in 1..slot_number)X[f][<i,j,1>] <= maximum_number*sum(a in Modulations)theta_incre[<i,j,1>][a];
 	  sum(f in 1..slot_number)X[f][<i,j,1>] >= sum(a in Modulations)theta_incre[<i,j,1>][a];
 	}
    //////////////////////////////////////
    //INCREMENTAL TRAFFIC 
    //eq(37), new traffic can be accommodated either by existing lightpaths and new lightpaths
 	forall(<ns,nd,nbw> in New_Requests, i in Nodes)
 	  if(i == ns)
 	    sum(j in Nodes:<i,j,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<i,j,1>] - sum(j in Nodes:<j,i,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<j,i,1>] == e_incre[<ns,nd,nbw>];
 	  else if(i == nd)
 	    sum(j in Nodes:<i,j,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<i,j,1>] - sum(j in Nodes:<j,i,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<j,i,1>] == -e_incre[<ns,nd,nbw>];
 	  else
 	    sum(j in Nodes:<i,j,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<i,j,1>] - sum(j in Nodes:<j,i,1> in New_Lightpaths)alpha_2[<ns,nd,nbw>][<j,i,1>] == 0;
    
    //eq(38), cancel loops electric
 	forall(r in New_Requests, j in Nodes)
 	  sum(i in Nodes: <i,j,1> in New_Lightpaths)alpha_2[r][<i,j,1>] <= 1; 
	
	//eq(39)
	forall(r in New_Requests, i in Nodes)
 	  sum(j in Nodes: <i,j,1> in New_Lightpaths)alpha_2[r][<i,j,1>] <= 1;
 	
 	//eq(40) 
 	forall(r in New_Requests, <i,j,1> in New_Lightpaths)
 	  alpha_2[r][<i,j,1>] + alpha_2[r][<j,i,1>]<= 1;
    
    //eq(42)
    forall(<ns,nd,nbw> in New_Requests)
    {
 	  sum(band in Bandwidth)rate[band][<ns,nd,nbw>] == 1;
 	  sum(band in Bandwidth)band*rate[band][<ns,nd,nbw>] <= nbw;
 	}
 	
 	//Eq(43)
 	forall(r in New_Requests)
 	{
 		e_incre[r] <= sum(band in Bandwidth)band*rate[band][r];
 		maximum_number*e_incre[r] >= sum(band in Bandwidth)band*rate[band][r];  	
 	}
 	
 	//Eq(44)
 	forall(<i,j,1> in New_Lightpaths)
 	  sum(<ns,nd,nbw> in New_Requests, band in Bandwidth)band*times*per[band][<ns,nd,nbw>][<i,j,1>] + sum(<ee,gg,0> in Lightpaths)Occupied_Lightpath_Capacity[<ee,gg,0>]*pai[<ee,gg,0>][<i,j>] <= spectrum_size*sum(f in 1..slot_number, a in Modulations)(sum(<ee,gg,0> in Lightpaths)a*rho_split[<ee,gg,0>][<i,j>][f][a] + a*rho[<i,j,1>][f][a]);
 	
 	//auxiliary	 	  
 	forall(<i,j,0> in Lightpaths, <x,y> in Split_Lps, f in 1..slot_number, a in Modulations)
 	{
 	  rho_split[<i,j,0>][<x,y>][f][a] >= X_split[f][<i,j,0>][<x,y>]+theta_split[<i,j,0>][<x,y>][a] - 1;  
 	  rho_split[<i,j,0>][<x,y>][f][a] <= X_split[f][<i,j,0>][<x,y>];
 	  rho_split[<i,j,0>][<x,y>][f][a] <= theta_split[<i,j,0>][<x,y>][a];
 	}
 	forall(<i,j,1> in New_Lightpaths, f in 1..slot_number, a in Modulations)
 	{	
 	  rho[<i,j,1>][f][a] >= X[f][<i,j,1>]+theta_incre[<i,j,1>][a] - 1;
 	  rho[<i,j,1>][f][a] <= X[f][<i,j,1>];
 	  rho[<i,j,1>][f][a] <= theta_incre[<i,j,1>][a];
 	}
 	forall(band in Bandwidth, <ns,nd,nbw> in New_Requests, <i,j,1> in New_Lightpaths)
 	{
 	  per[band][<ns,nd,nbw>][<i,j,1>] >= rate[band][<ns,nd,nbw>] + alpha_2[<ns,nd,nbw>][<i,j,1>] - 1;
 	  per[band][<ns,nd,nbw>][<i,j,1>] <= alpha_2[<ns,nd,nbw>][<i,j,1>];
 	  per[band][<ns,nd,nbw>][<i,j,1>] <= rate[band][<ns,nd,nbw>];
 	}	  
};