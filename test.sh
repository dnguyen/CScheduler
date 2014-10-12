POLICIES=("fcfs" "srtf" "pbs" "mlfq")
POLICY_NUM=0
for policy in "${POLICIES[@]}"
do
    for i in 0 1 2 3
    do
        echo "[Testing $policy input_$i]"
        ./out $POLICY_NUM p1_test/TestInputs/input_$i > outputs/$policy_input$i.txt
        MINE="outputs/$policy_input$i.txt"
        CORRECT="p1_test/TestOutputs/base_input_"
        CORRECT+=$i
        CORRECT+="_"
        CORRECT+=$policy
        CORRECT+="_output"
        echo -e "\t[diff $MINE $CORRECT"

        if diff "$MINE" "$CORRECT" &> /dev/null ; then
            echo -e "\t\e[1;34m[PASS]\e[0m"
        else
            echo -e "\t\e[1;31m[FAIL]\e[0m"
        fi
    done
    ((POLICY_NUM++))
done