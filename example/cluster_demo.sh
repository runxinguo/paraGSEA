#!/bin/bash
read -p "Please enter the way to calculate the ES_Matrix(0_nocom,1_p2p,2_cocom):" matrix_way
read -p "Please enter the way to execute clustering(0_KMediods,1_KMediods++):" cluster_way

# execute matlab script to parse the data(pre_treatment)
runPreGSEAbyMatlab.sh
#runparaPreGSEAbyMatlab.sh

outputfile="../data/Cluster_result_test.txt"
n=3
ppn=3
thread_num=5
siglen=50
cluster_num=5

#calculate the similarity(ES) matrix
case "$matrix_way" in
	0)mpirun -n $n -ppn $ppn -hostfile hostfile ES_Matrix_ompi_nocom -t $thread_num -l $siglen -1 "../data/data_for_test.txt" -2 "../data/data_for_test.txt" -o "../data/ES_Matrix_tmp";;
	1)mpirun -n $n -ppn $ppn -hostfile hostfile ES_Matrix_ompi_p2p -t $thread_num -l $siglen -1 "../data/data_for_test.txt" -2 "../data/data_for_test.txt" -o "../data/ES_Matrix_tmp";;
	2)mpirun -n $n -ppn $ppn -hostfile hostfile ES_Matrix_ompi_cocom -t $thread_num -l $siglen -1 "../data/data_for_test.txt" -2 "../data/data_for_test.txt" -o "../data/ES_Matrix_tmp";;
esac

#cluster
case "$cluster_way" in
	0)mpirun -n $n -ppn $ppn -hostfile hostfile Cluster_KMediods_ompi -t $thread_num -c $cluster_num -i "../data/ES_Matrix_tmp" -o $outputfile -s ../data/data_for_test_cidnum.txt -r ../data/Reference;;
	1)mpirun -n $n -ppn $ppn -hostfile hostfile Cluster_KMediods++_ompi -t $thread_num -c $cluster_num -i "../data/ES_Matrix_tmp" -o $outputfile -s ../data/data_for_test_cidnum.txt -r ../data/Reference;;
esac