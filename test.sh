echo "==== TESTING FIRST COME FIRST SERVE INPUT_1 ===="
./out 0 p1_test/TestInputs/input_1 > outputs/fcfs_input1.txt
diff -q "outputs/fcfs_input1.txt" "p1_test/TestOutputs/base_input_1_fcfs_output"

echo "==== TESTING FIRST COME FIRST SERVE INPUT_2 ===="
./out 0 p1_test/TestInputs/input_2 > outputs/fcfs_input2.txt
diff -q "outputs/fcfs_input2.txt" "p1_test/TestOutputs/base_input_2_fcfs_output"

echo "==== TESTING FIRST COME FIRST SERVE INPUT_3 ===="
./out 0 p1_test/TestInputs/input_3 > outputs/fcfs_input3.txt
diff -q "outputs/fcfs_input3.txt" "p1_test/TestOutputs/base_input_3_fcfs_output"

echo "==== TESTING SRTF INPUT_1 ===="
./out 1 p1_test/TestInputs/input_1 > outputs/srtf_input1.txt
diff -q "outputs/srtf_input1.txt" "p1_test/TestOutputs/base_input_1_srtf_output"

echo "==== TESTING SRTF INPUT_2 ===="
./out 1 p1_test/TestInputs/input_2 > outputs/srtf_input2.txt
diff -q "outputs/srtf_input3.txt" "p1_test/TestOutputs/base_input_3_srtf_output"

echo "==== TESTING SRTF INPUT_3 ===="
./out 1 p1_test/TestInputs/input_3 > outputs/srtf_input3.txt
diff -q "outputs/srtf_input3.txt" "p1_test/TestOutputs/base_input_3_srtf_output"

echo "==== TESTING MLFQ INPUT_1 ===="
#./out 2 p1_test/TestInputs/input_1
