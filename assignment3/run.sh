# ============================================
# Brendan Moore | OS Section 04
# ============================================

#!/bin/bash
echo "Compiling and running producer and consumer with predefined values."
echo "|"

# Compile producer and consumer
gcc -Wall -pthread -lrt producer.c -o producer.o
gcc -Wall -pthread -lrt consumer.c -o consumer.o

# 1 producer and 1 consumer run in parallel
./producer.o 1 5 &
./consumer.o 1 5 &

wait
echo "|"
echo "> 1 producer and 1 consumer run in parallel -|- Test Complete. <"
echo "|"

# 1 producer and multiple consumers running in parallel
./producer.o 1 10 &

for i in {1..5}
do
    ./consumer.o $i 2 &
done

wait
echo "|"
echo "> 1 producer and multiple consumers running in parallel -|- Test Complete. <"
echo "|"

# Multiple producers running in parallel with 1 consumer
for i in {1..5}
do
    ./producer.o $i 2 &
done

./consumer.o 1 10 &

wait
echo "|"
echo "> Multiple producers running in parallel with 1 consumer -|- Test Complete. <"
echo "|"

# Clean up shared memory
ipcrm -M 0x1234