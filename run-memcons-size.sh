#/bin/bash

OPTIONS="-a -l 1 -L 10 -t 10000000 -i 0 -d 30 -n 1"
REPS=5
echo "#Created at: `date` on `hostname`"
echo "#Average on $REPS runs. ACDC Options: $OPTIONS"

echo -e "#lower_sc\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev"
#for THREADS in 1 2 3 4 5 6 7 8
for XVALUE in 3 4 5 6 7 8 9 10
do
	OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		VALUE_SUM=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			let "LOWER=$XVALUE"
			let "HIGHER=$XVALUE+1"
			RESULT=$(./build/acdc-$CONF $OPTIONS -r $REP -s $LOWER -S $HIGHER | grep MEMORY)
			read -a ARRAY <<<$RESULT
			#DIVISOR=$(echo "2 ^ $LOWER"| bc)
			DIVISOR=1
			VALUE[$REP]=$(echo "scale=1; ${ARRAY[5]} / $DIVISOR" | bc)
			#VALUE[$REP]=${ARRAY[5]}
			VALUE_SUM=$(echo "$VALUE_SUM + ${VALUE[$REP]}" | bc)
		done
		AVG=$(echo "scale=1;$VALUE_SUM / $REPS" | bc)
		SSD=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			SSD=$(echo "$SSD + (${VALUE[$REP]} - $AVG)^2"|bc)
		done
		SSD=$(echo "scale=1;sqrt($SSD * (1 / ($REPS - 1)))"|bc)
		OUTPUT="$OUTPUT\t$AVG\t$SSD"
	done
	echo -e $OUTPUT
done

