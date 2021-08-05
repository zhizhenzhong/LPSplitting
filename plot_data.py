import os
import matplotlib.pyplot as plt
import numpy as np
import re
import scipy as sp
import scipy.stats

from pylab import *

doc_10 = open('fig8.txt', 'w')
doc_11 = open('fig9.txt', 'w')
doc_12 = open('fig10.txt', 'w')
doc_13 = open('fig11.txt', 'w')
doc_14 = open('fig12.txt', 'w')
doc_15 = open('fig13.txt', 'w')
doc_16 = open('fig14.txt', 'w')

def _tqdm(iterable, desc=None):
    return iterable

def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return h

_DATA_DIR = os.path.expanduser('~/Documents/Lightpath-Splitting/Experiment/final_results')

K_number = 15 #10, 20, 30, ..., 140, 150

data_location = os.path.join(_DATA_DIR, 'output-f-mu-1-res1-1.dat')

# Read data from all files
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    location_1 = data_location.replace('mu-1', mu)
    locals()['incremental_load' + mu] = 0

    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO', 'All_MaxE', 'All_MaxO', 'None']:
        locals()[algorithm + '_throughput_' + mu] = []
        locals()[algorithm + '_incre_throughput_' + mu] = []
        locals()[algorithm + '_affect_bw_' + mu] = []
        locals()[algorithm + '_unaffect_bw_' + mu] = []
        locals()[algorithm + '_hops_' + mu] = []

        locals()['Err_' + algorithm + '_throughput_' + mu] = []
        locals()['Err_' + algorithm + '_incre_throughput_' + mu] = []
        locals()['Err_' + algorithm + '_affect_bw_' + mu] = []
        locals()['Err_' + algorithm + '_unaffect_bw_' + mu] = []
        locals()['Err_' + algorithm + '_hops_' + mu] = []

        locals()['baseline_throughput' + mu] = []
        locals()['incre_throughput' + mu] = []
        locals()['None_throughput_150K' + mu] = []
        locals()['Err_None_throughput_150K' + mu] = []

        for K in range(0, 16, 1):
            locals()[algorithm + '_throughput_' + mu + str(K)] = []
            locals()[algorithm + '_incre_throughput_' + mu + str(K)] = []
            locals()[algorithm + '_affect_bw_' + mu + str(K)] = []
            locals()[algorithm + '_unaffect_bw_' + mu + str(K)] = []
            locals()[algorithm + '_hops_' + mu + str(K)] = []
            for node_index in ['n1', 'n2', 'n3', 'n4', 'n5', 'n6', 'n7', 'n8', 'n9', 'n10', 'n11', 'n12', 'n13', 'n14']:
                locals()[algorithm + '_transceiver_' + mu + node_index + '_' + str(K)] = []

    #Read data from files within a mu value
    for round in ['res-1', 'res-2', 'res-3', 'res-4', 'res-5', 'res-6', 'res-7', 'res-8', 'res-9', 'res-0']:
        location_2 = location_1.replace('res-1', round)

        for repeat in ['-1.dat', '-2.dat', '-3.dat', '-4.dat', '-5.dat', '-6.dat', '-7.dat', '-8.dat', '-9.dat', '-0.dat']:
            real_data_location = location_2.replace('-1.dat', repeat)

            locals()['throughput_' + mu + repeat + round] = []
            locals()['affect_bw_' + mu + repeat + round] = []
            locals()['ave_hops_' + mu + repeat + round] = []
            locals()['incre_hops_bw_' + mu + repeat + round] = []
            for node_index in ['n1', 'n2', 'n3', 'n4', 'n5', 'n6', 'n7', 'n8', 'n9', 'n10', 'n11', 'n12', 'n13', 'n14']:
                locals()['transceiver_num' + mu + repeat + round + node_index] = []
            base_incre_ave_hops = []

            count = 0
            with open(real_data_location, 'r', encoding='gb2312') as fr:
                for line in _tqdm(fr, desc='loading'):
                    if re.match(r'((-)?\d+\s+)+\d+', line):
                        if int(line.split()[0]) > 12:
                            locals()['throughput_' + mu + repeat + round].append(6.25*int(line.split()[0]))
                            locals()['affect_bw_' + mu + repeat + round].append(6.25*int(line.split()[1]))
                            locals()['ave_hops_' + mu + repeat + round].append(float(line.split()[2]))
                            locals()['incre_hops_bw_' + mu + repeat + round].append(6.25*int(line.split()[3]))
                            count += 1

                    if line.startswith('*Baseline'):
                        locals()['baseline_throughput' + mu + repeat + round] = 6.25*int(line.split()[1])

                    if line.startswith('*Incremental'):
                        locals()['incre_throughput' + mu + repeat + round] = 6.25*int(line.split()[1])

                    if line.startswith('*平均bps'):
                        base_incre_ave_hops.append(float(line.split()[1]))

                    if line.startswith('incremental(mu'):
                        locals()['incremental_load' + mu] = 6.25*int(line.split()[1])

                    if line.startswith('Incremental成功接入业务数是:'):
                         locals()['splitpoints_num' + mu + repeat + round] = int(line.split()[6])

                    if line.startswith('SplitPoint'):
                        count_node = 2
                        for node_index in ['n1', 'n2', 'n3', 'n4', 'n5', 'n6', 'n7', 'n8', 'n9', 'n10', 'n11', 'n12', 'n13', 'n14']:
                            locals()['transceiver_num' + mu + repeat + round + node_index].append(int(line.split()[count_node]))
                            count_node = count_node + 1

            locals()['baseline_ave_hops' + mu + repeat + round] = base_incre_ave_hops[0]
            locals()['incre_ave_hops' + mu + repeat + round] = base_incre_ave_hops[1]

    # calculate the average and error of the value
    locals()['average_splitpoint' + mu] = []
    map_agrm = {0: 'BF_MaxE', 1: 'DF_MaxE', 2: 'BF_MaxO', 3: 'DF_MaxO', 4: 'SA_MaxE', 5: 'SA_MaxO', 90: 'All_MaxE',91: 'All_MaxO'}
    for round in ['res-1', 'res-2', 'res-3', 'res-4', 'res-5', 'res-6', 'res-7', 'res-8', 'res-9', 'res-0']:
        for repeat in ['-1.dat', '-2.dat', '-3.dat', '-4.dat', '-5.dat', '-6.dat', '-7.dat', '-8.dat', '-9.dat', '-0.dat']:

            locals()['average_splitpoint' + mu].append(locals()['splitpoints_num' + mu + repeat + round])
            locals()['baseline_throughput' + mu].append(locals()['baseline_throughput' + mu + repeat + round])
            locals()['incre_throughput' + mu].append(locals()['incre_throughput' + mu + repeat + round])
            locals()['None_throughput_150K' + mu].append(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])

            for al in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
                locals()[al + '_throughput_' + mu + str(0)].append(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])
                locals()[al + '_incre_throughput_' + mu + str(0)].append(locals()['incre_throughput' + mu + repeat + round]/locals()['incremental_load' + mu])
                locals()[al + '_hops_' + mu + str(0)].append((locals()['baseline_ave_hops' + mu + repeat + round]*locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_ave_hops' + mu + repeat + round]*locals()['incre_throughput' + mu + repeat + round])/(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round]))

            for index in range(count):
                if index < 90:
                    locals()[map_agrm[index % 6] + '_throughput_' + mu + str(1 + index // 6)].append(locals()['throughput_' + mu + repeat + round][index] + locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])
                    locals()[map_agrm[index % 6] + '_incre_throughput_' + mu + str(1 + index // 6)].append((locals()['throughput_' + mu + repeat + round][index]+locals()['incre_throughput' + mu + repeat + round])/locals()['incremental_load' + mu])
                    locals()[map_agrm[index % 6] + '_affect_bw_' + mu + str(1 + index // 6)].append(locals()['affect_bw_' + mu + repeat + round][index]/(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round]))
                    locals()[map_agrm[index % 6] + '_unaffect_bw_' + mu + str(1 + index // 6)].append(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round] - locals()['affect_bw_' + mu + repeat + round][index])
                    locals()[map_agrm[index % 6] + '_hops_' + mu + str(1 + index // 6)].append(
                        (locals()['ave_hops_' + mu + repeat + round][index]*locals()['throughput_' + mu + repeat + round][index]
                        + locals()['baseline_ave_hops' + mu + repeat + round]*locals()['baseline_throughput' + mu + repeat + round]
                        + locals()['incre_ave_hops' + mu + repeat + round]*locals()['incre_throughput' + mu + repeat + round]
                        + locals()['incre_hops_bw_' + mu + repeat + round][index])
                        / (locals()['throughput_' + mu + repeat + round][index] + locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])
                    )
                    for node_index in ['n1', 'n2', 'n3', 'n4', 'n5', 'n6', 'n7', 'n8', 'n9', 'n10', 'n11', 'n12', 'n13', 'n14']:
                        locals()[map_agrm[index % 6] + '_transceiver_' + mu + node_index + '_' + str(1 + index // 6)].append(locals()['transceiver_num' + mu + repeat + round + node_index][index])

                else:
                    locals()[map_agrm[index] + '_throughput_' + mu].append(locals()['throughput_' + mu + repeat + round][index] + locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])
                    locals()[map_agrm[index] + '_incre_throughput_' + mu].append((locals()['throughput_' + mu + repeat + round][index]+locals()['incre_throughput' + mu + repeat + round])/locals()['incremental_load' + mu])
                    locals()[map_agrm[index] + '_affect_bw_' + mu].append(locals()['affect_bw_' + mu + repeat + round][index]/(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round]))
                    locals()[map_agrm[index] + '_unaffect_bw_' + mu].append(locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round] - locals()['affect_bw_' + mu + repeat + round][index])
                    locals()[map_agrm[index] + '_hops_' + mu].append(
                        (locals()['ave_hops_' + mu + repeat + round][index] *locals()['throughput_' + mu + repeat + round][index]
                         + locals()['baseline_ave_hops' + mu + repeat + round] * locals()['baseline_throughput' + mu + repeat + round]
                         + locals()['incre_ave_hops' + mu + repeat + round] * locals()['incre_throughput' + mu + repeat + round]
                         + locals()['incre_hops_bw_' + mu + repeat + round][index])
                        / (locals()['throughput_' + mu + repeat + round][index] + locals()['baseline_throughput' + mu + repeat + round] + locals()['incre_throughput' + mu + repeat + round])
                    )

    average_splitpoints = float(sum(locals()['average_splitpoint' + mu]) / len(locals()['average_splitpoint' + mu]))
    err_ave_splitpoints = mean_confidence_interval(locals()['average_splitpoint' + mu], 0.95)
    print(mu, 'Average splitpoints number:', average_splitpoints, err_ave_splitpoints)

######################################################
#Fig. 8, overall network throughput vs. mu
######################################################
for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO', 'All_MaxE', 'All_MaxO', 'None']:
    locals()[algorithm + '_throughput_150K'] = []
    locals()['Err' + algorithm + '_throughput_150K'] = []

    seq = []
    for mu in ['mu-1', 'mu-3', 'mu-5']:
        seq += locals()['baseline_throughput' + mu]

    locals()[algorithm + '_throughput_150K'].append(float(sum(seq)/len(seq)))
    locals()['Err' + algorithm + '_throughput_150K'].append(mean_confidence_interval(seq, 0.95))

    ## generate data for throughput vs. mu
for mu in ['mu-1', 'mu-2', 'mu-3', 'mu-4', 'mu-5']:
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        locals()[algorithm + '_throughput_150K'].append(float(sum(locals()[algorithm + '_throughput_' + mu + '15'])/len(locals()[algorithm + '_throughput_' + mu + '15'])))
        locals()['Err' + algorithm + '_throughput_150K'].append(mean_confidence_interval(locals()[algorithm + '_throughput_' + mu + '15'], 0.95))

    for algorithm in ['All_MaxE', 'All_MaxO']:
        locals()[algorithm + '_throughput_150K'].append(float(sum(locals()[algorithm + '_throughput_' + mu])/len(locals()[algorithm + '_throughput_' + mu])))
        locals()['Err' + algorithm + '_throughput_150K'].append(mean_confidence_interval(locals()[algorithm + '_throughput_' + mu], 0.95))
    for algorithm in ['None']:
        locals()[algorithm + '_throughput_150K'].append(float(sum(locals()['None_throughput_150K' + mu])/len(locals()['None_throughput_150K' + mu])))
        locals()['Err' + algorithm + '_throughput_150K'].append(mean_confidence_interval(locals()['None_throughput_150K' + mu], 0.95))

mu_value = [0,1,2,3,4,5] # used to be [0,1,2,3,4,5]

plt.figure()
getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'All_MaxE': 'grey', 'All_MaxO': 'grey', 'None': 'violet'}
error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO', 'All_MaxE', 'All_MaxO', 'None']:
    #print(len(splitpoint), splitpoint)
    print(method, len(locals()[method + '_throughput_150K']), locals()[method + '_throughput_150K'], file=doc_10)
    print('Err', method, len(locals()['Err' + method + '_throughput_150K']), locals()['Err' + method + '_throughput_150K'], file=doc_10)
    throughput_mu = plt.errorbar(mu_value, locals()[method + '_throughput_150K'], locals()['Err' + method + '_throughput_150K'], linewidth=1, elinewidth=1, capsize=1,marker='o', color=getcolors[method], markersize=2, label=str(method))

plt.legend(loc='upper left', fontsize=12)
plt.grid(True)
plt.xlabel("Incremental index", fontsize=12)
plt.ylabel("Overall network throughput", fontsize=12)

plt.xticks(fontsize=12)
plt.yticks(fontsize=12)
    #plt.xlim(13, 25)
    #plt.ylim(0.86, 1)
plt.tight_layout()
plt.savefig('fig8.pdf', dpi=175)


######################################################
#Fig. 9 overall network throughput vs. splitpoints
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    ## generate data for throughput vs. splitplints
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        for nn in range(0, 16, 1):
            locals()[algorithm + '_throughput_' + mu].append(float(sum(locals()[algorithm + '_throughput_' + mu + str(nn)]) / len(locals()[algorithm + '_throughput_' + mu + str(nn)])))
            locals()['Err_' + algorithm + '_throughput_' + mu].append(mean_confidence_interval(locals()[algorithm + '_throughput_' + mu + str(nn)], 0.95))
        print('MU:', mu, file=doc_11)
        print(algorithm, locals()[algorithm + '_throughput_' + mu], file=doc_11)
        print('Err', algorithm, locals()['Err_' + algorithm + '_throughput_' + mu], file=doc_11)

    for algorithm in ['All_MaxE', 'All_MaxO']:
        locals()['average_' + algorithm] = float(sum(locals()[algorithm + '_throughput_' + mu])/len(locals()[algorithm + '_throughput_' + mu]))
        print('Average ' +  algorithm, locals()['average_' + algorithm], file=doc_11)

        locals()['Err_average_' + algorithm] = mean_confidence_interval(locals()[algorithm + '_throughput_' + mu], 0.95)
        print('Err_Average ' + algorithm, locals()['Err_average_' + algorithm], file=doc_11)

    #for algorithm in ['All_MaxE', 'All_MaxO']:
        #locals()[algorithm + '_throughput_' + mu].append(float(sum(locals()[algorithm + '_throughput_' + mu]) / len(locals()[algorithm + '_throughput_' + mu])))
        #locals()['Err_' + algorithm + '_throughput_' + mu].append(mean_confidence_interval(locals()[algorithm + '_throughput_' + mu], 0.95))

    splitpoint = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
    plt.figure()
    getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
    getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
    error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        #print(len(splitpoint), splitpoint)
        #print(len(locals()[method + '_throughput_' + mu]), locals()[method + '_throughput_' + mu])
        throughput_mu = plt.errorbar(splitpoint, locals()[method + '_throughput_' + mu], locals()['Err_' + method + '_throughput_' + mu], linewidth=1, elinewidth=1, capsize=1,marker='o', color=getcolors[method], markersize=2, label=str(method))

    plt.legend(loc='lower right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Splitpoints", fontsize=12)
    plt.ylabel("Overall network throughput", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    #plt.xlim(13, 25)
    #plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig9-'+ mu +'.pdf', dpi=175)


######################################################
#Fig. 10 normalized incremental traffic vs. splitpoints
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    ## generate data for throughput vs. splitplints
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        for nn in range(0, 16, 1):
            locals()[algorithm + '_incre_throughput_' + mu].append(float(sum(locals()[algorithm + '_incre_throughput_' + mu + str(nn)]) / len(locals()[algorithm + '_incre_throughput_' + mu + str(nn)])))
            locals()['Err_' + algorithm + '_incre_throughput_' + mu].append(mean_confidence_interval(locals()[algorithm + '_incre_throughput_' + mu + str(nn)], 0.95))
        print('MU:', mu, file=doc_12)
        print(algorithm, locals()[algorithm + '_incre_throughput_' + mu], file=doc_12)
        print('Err', algorithm, locals()['Err_' + algorithm + '_incre_throughput_' + mu], file=doc_12)

    #for algorithm in ['All_MaxE', 'All_MaxO']:
        #locals()[algorithm + '_incre_throughput_' + mu].append(float(sum(locals()[algorithm + '_incre_throughput_' + mu]) / len(locals()[algorithm + '_incre_throughput_' + mu])))
        #locals()['Err_' + algorithm + '_incre_throughput_' + mu].append(mean_confidence_interval(locals()[algorithm + '_incre_throughput_' + mu], 0.95))

    splitpoint = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
    plt.figure()
    getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
    getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
    error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        #print(len(splitpoint), splitpoint)
        #print(len(locals()[method + '_throughput_' + mu]), locals()[method + '_throughput_' + mu])
        throughput_mu = plt.errorbar(splitpoint, locals()[method + '_incre_throughput_' + mu], locals()['Err_' + method + '_incre_throughput_' + mu], linewidth=1, elinewidth=1, capsize=1,marker='o', color=getcolors[method], markersize=2, label=str(method))

    plt.legend(loc='lower right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Splitpoints", fontsize=12)
    plt.ylabel("Normalized incremental throughput", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    #plt.xlim(13, 25)
    #plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig10-' + mu +'.pdf', dpi=175)


######################################################
#Fig. 11, normalized affected traffic vs. splitpoints
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        locals()[algorithm + '_affect_bw_' + mu].append(0)
        locals()['Err_' + algorithm + '_affect_bw_' + mu].append(0)
        for nn in range(1, 16, 1):
            locals()[algorithm + '_affect_bw_' + mu].append(float(sum(locals()[algorithm + '_affect_bw_' + mu + str(nn)]) / len(locals()[algorithm + '_affect_bw_' + mu + str(nn)])))
            locals()['Err_' + algorithm + '_affect_bw_' + mu].append(mean_confidence_interval(locals()[algorithm + '_affect_bw_' + mu + str(nn)], 0.95))

        print('MU:', mu, file=doc_13)
        print(algorithm, locals()[algorithm + '_affect_bw_' + mu], file=doc_13)
        print('Err'+algorithm, locals()['Err_' + algorithm + '_affect_bw_' + mu], file=doc_13)

    for algorithm in ['All_MaxE', 'All_MaxO']:
        locals()['average_affect_' + algorithm] = float(sum(locals()[algorithm + '_affect_bw_' + mu])/len(locals()[algorithm + '_affect_bw_' + mu]))
        locals()['Err_average_affect_' + algorithm] = mean_confidence_interval(locals()[algorithm + '_affect_bw_' + mu], 0.95)

        print('All '+algorithm, locals()['average_affect_' + algorithm], file=doc_13)
        print('All_Error ' + algorithm, locals()['Err_average_affect_' + algorithm], file=doc_13)

    splitpoint = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
    plt.figure()
    getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
    getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
    error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        # print(len(splitpoint), splitpoint)
        # print(len(locals()[method + '_throughput_' + mu]), locals()[method + '_throughput_' + mu])
        throughput_mu = plt.errorbar(splitpoint, locals()[method + '_affect_bw_' + mu], locals()['Err_' + method + '_affect_bw_' + mu], linewidth=1, elinewidth=1,capsize=1, marker='o', color=getcolors[method], markersize=2, label=str(method))

    plt.legend(loc='lower right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Splitpoints", fontsize=12)
    plt.ylabel("Normalized affected traffic", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    # plt.xlim(13, 25)
    # plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig11-' + mu + '.pdf', dpi=175)


######################################################
#Fig. 12, average trasceivers needed per node, when mu = 2
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        locals()[algorithm + '_transceiver_' + mu + '_12'] = []
        locals()['Err' + algorithm  + '_transceiver_' + mu + '_12'] = []
        for node_index in ['n1', 'n2', 'n3', 'n4', 'n5', 'n6', 'n7', 'n8', 'n9', 'n10', 'n11', 'n12', 'n13', 'n14']:
            #print(node_index, '=', len(locals()[algorithm + '_transceiver_' + 'mu-2' + node_index + '_12']))
            locals()[algorithm + '_transceiver_' + mu + '_12'].append(sum(locals()[algorithm + '_transceiver_' + mu + node_index + '_12']) / len(locals()[algorithm + '_transceiver_' + mu + node_index + '_12']))
            locals()['Err' + algorithm + '_transceiver_' + mu + '_12'].append(mean_confidence_interval(locals()[algorithm + '_transceiver_' + mu + node_index + '_12'], 0.95))

        print('MU:', mu, file=doc_14)
        print(algorithm, locals()[algorithm + '_transceiver_' + mu + '_12'], file=doc_14)
        print('Err' + algorithm, locals()['Err' + algorithm + '_transceiver_' + mu + '_12'], file=doc_14)

node_set = [1,2,3,4,5,6,7,8,9,10,11,12,13,14]
node_set = np.array(node_set)
offset = {'BF_MaxE': -0.25, 'DF_MaxE': -0.15, 'BF_MaxO': -0.05, 'DF_MaxO': 0.05, 'SA_MaxE': 0.15,'SA_MaxO': 0.25}
getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}
bar_width = 0.1
opacity = 1

for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    plt.figure()
    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        #print(len(node_set), node_set)
        print(len(locals()[method + '_transceiver_' + mu + '_12']), locals()[method + '_transceiver_' + mu + '_12'])
        locals()['transceiver' + method] = plt.bar(node_set + offset[method], locals()[method + '_transceiver_' + mu + '_12'], bar_width, alpha=opacity,
                                color=getcolors[method],
                                yerr=locals()['Err' + method  + '_transceiver_' + mu + '_12'],
                                error_kw=error_config,
                                label=method
                               )
    plt.legend(loc='upper right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Node Index", fontsize=12)
    plt.ylabel("Average Transceivers", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    # plt.xlim(13, 25)
    # plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig12-' + mu + '-120K.pdf', dpi=175)


######################################################
#Fig. 13, average traffic hops per b/s vs. splitpoints
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    print(mu, file = doc_15)
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        for nn in range(0, 16, 1):
            locals()[algorithm + '_hops_' + mu].append(float(sum(locals()[algorithm + '_hops_' + mu + str(nn)])/len(locals()[algorithm + '_hops_' + mu + str(nn)])))
            locals()['Err_' + algorithm + '_hops_' + mu].append(mean_confidence_interval(locals()[algorithm + '_hops_' + mu + str(nn)], 0.95))

        print('Hops ' + algorithm, locals()[algorithm + '_hops_' + mu], file=doc_15)
        print('HopsError ' + algorithm, locals()['Err_' + algorithm + '_hops_' + mu], file=doc_15)

    splitpoint = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

    for method in ['All_MaxE', 'All_MaxO']:
        locals()[method + '_all_hops_' + mu] = []
        locals()['Err' + method + '_all_hops_' + mu] = []
        for nn in range(0, 16, 1):
            locals()[method + '_all_hops_' + mu].append(float(sum(locals()[method + '_hops_' + mu]) / len(locals()[method + '_hops_' + mu])))
            locals()['Err' + method + '_all_hops_' + mu].append(mean_confidence_interval(locals()[method + '_hops_' + mu], 0.95))
        print(method, locals()[method + '_all_hops_' + mu], file=doc_15)
        print('Err'+method, locals()['Err' + method + '_all_hops_' + mu], file=doc_15)


    plt.figure()
    getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
    getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
    error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        print(len(locals()[method + '_hops_' + mu]), locals()[method + '_hops_' + mu])
        print(len(locals()[method + '_hops_' + mu]), locals()[method + '_hops_' + mu])
        throughput_mu = plt.errorbar(splitpoint, locals()[method + '_hops_' + mu], locals()['Err_' + method + '_hops_' + mu], linewidth=1, elinewidth=1,capsize=1, marker='o', color=getcolors[method], markersize=2, label=str(method))

    for method in ['All_MaxE', 'All_MaxO']:
        fig = plot(splitpoint, locals()[method + '_all_hops_' + mu])

    plt.legend(loc='lower right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Splitpoints", fontsize=12)
    plt.ylabel("Average traffic hops", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    # plt.xlim(13, 25)
    # plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig13-' + mu + '.pdf', dpi=175)


######################################################
#Fig. 14, overall network throughput vs. unaffected traffic volume
######################################################
for mu in ['mu-1', 'mu-2','mu-3', 'mu-4', 'mu-5']:
    for algorithm in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        locals()[algorithm + '_unaffect_bw_' + mu].append(float(sum(locals()[algorithm + '_throughput_' + mu + '0'])/len(locals()[algorithm + '_throughput_' + mu + '0'])))
        locals()['Err_' + algorithm + '_unaffect_bw_' + mu].append(mean_confidence_interval(locals()[algorithm + '_throughput_' + mu + '0'], 0.95))

        for nn in range(1, 16, 1):
            locals()[algorithm + '_unaffect_bw_' + mu].append(float(sum(locals()[algorithm + '_unaffect_bw_' + mu + str(nn)])/len(locals()[algorithm + '_unaffect_bw_' + mu + str(nn)])))
            locals()['Err_' + algorithm + '_unaffect_bw_' + mu].append(mean_confidence_interval(locals()[algorithm + '_unaffect_bw_' + mu + str(nn)], 0.95))

    plt.figure()
    getcolors = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red', 'None': 'indigo'}
    getmarkers = {'BF_MaxE': 'blue', 'DF_MaxE': 'green', 'BF_MaxO': 'blue', 'DF_MaxO': 'green', 'SA_MaxE': 'red','SA_MaxO': 'red'}
    error_config = {'ecolor': 'black', 'capsize': 1, 'linewidth': 1}

    for method in ['BF_MaxE', 'DF_MaxE', 'BF_MaxO', 'DF_MaxO', 'SA_MaxE', 'SA_MaxO']:
        print(mu, method, len(locals()[method + '_unaffect_bw_' + mu]), locals()[method + '_unaffect_bw_' + mu],file=doc_16)
        print(mu, method, len(locals()['Err_' + method + '_unaffect_bw_' + mu]),locals()['Err_' + method + '_unaffect_bw_' + mu], file=doc_16)

        print(mu, method, len(locals()[method + '_throughput_' + mu]), locals()[method + '_throughput_' + mu], file=doc_16)
        print(mu, method, len(locals()['Err_' + method + '_throughput_' + mu]), locals()['Err_' + method + '_throughput_' + mu], file=doc_16)

        throughput_mu = plt.errorbar(locals()[method + '_unaffect_bw_' + mu], locals()[method + '_throughput_' + mu], locals()['Err_' + method + '_throughput_' + mu], locals()['Err_' + method + '_unaffect_bw_' + mu], linewidth=1, elinewidth=1,capsize=1, marker='o', color=getcolors[method], markersize=2, label=str(method))

    for method in ['All_MaxE', 'All_MaxO']:
        locals()['average_' + method] = float(sum(locals()[method + '_throughput_' + mu]) / len(locals()[method + '_throughput_' + mu]))
        locals()['Err_average_' + method] = mean_confidence_interval(locals()[method + '_throughput_' + mu], 0.95)

        #locals()['average_' + algorithm] = float(sum(locals()[algorithm + '_throughput_' + mu]) / len(locals()[algorithm + '_throughput_' + mu]))
        print('Average ' + method, locals()['average_' + method], file=doc_16)
        print('Err_Average ' + method, locals()['Err_average_' + method], file=doc_16)

        locals()['Average_unaffected' + method] = float(sum(locals()[method + '_unaffect_bw_' + mu])/len(locals()[method + '_unaffect_bw_' + mu]))
        locals()['Err_Average_unaffected' + method] = mean_confidence_interval(locals()[method + '_unaffect_bw_' + mu], 0.95)

        print('All unaffected ' + method, locals()['Average_unaffected' + method], file=doc_16)
        print('All unaffected Error ' + method, locals()['Err_Average_unaffected' + method], file=doc_16)

    plt.legend(loc='lower right', fontsize=12)
    plt.grid(True)
    plt.xlabel("Unaffected traffic", fontsize=12)
    plt.ylabel("Overall network throughput", fontsize=12)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    # plt.xlim(13, 25)
    # plt.ylim(0.86, 1)
    plt.tight_layout()
    plt.savefig('fig14-' + mu + '.pdf', dpi=175)
