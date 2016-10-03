<a name="ES_Matrix_ompi_cocom.doc"></a>
# ES_Matrix_ompi_cocom #

This tool Compares two gene profile sets to get an Enrichment Score matrix 
of every gene profile pairs and write out the result by MPI/OpenMP with collective
communication when distribute the input file.

There are several parameters should be setted in command line example.

| parameter name | Parameter function |
| -------------- | -------------------|
| -n process_num | Total number of processes |
| -ppn pernum |the number of processes in each node |
| -hostfile hostfile | list the IP or Hostname of nodes |
| -t --thread | define the maximum number of parallel threads |
| -l--siglen | define the length of Gene Expression Signatur |
| -1 --input1 | a parsed profiles's file from pretreatment stage |
| -2 --input2 | another parsed profiles's file from pretreatment stage |
| -o --output | output file ,distributed in every nodes ,with ES Matrix |

A sample `Shell script` file is given below that makes use of `ES_Matrix_ompi_cocom`.

```shell
mpirun -n 2 -ppn 2 -hostfile example/hostfile ES_Matrix_ompi_cocom -t 4 -l 50 -1 data/data_for_test.txt -2 data/data_for_test.txt -o data/ES_Matrix_test
```

the example hostfile just has one record as below:
```shell
localhost
```

However, there is a thing we must notice:
you'd better keep the formula `process_num = pernum * number of IP(Hostname) list in hostfile` 
correct.

It may will produce the following output:
```shell
Profile Set is Loading...!
loading IO and prework time in collective comunication: 0.4318 s
Paral compute the ES_Matrix is Starting...!
Paral compute the ES_Matrix time : 2.8079 s
Writing file is Starting...!
Write Result spent: 0.0306 s
```

There is no more need of you to input anything in command line. However,
the ES_Matrix will be written to file `data/ES_Matrix_test_*.txt` in every
processes in a distributed way.