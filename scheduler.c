#include <stdio.h>

FILE *logger;

void init_scheduler(int sched_type) {
    logger = fopen("log.txt", "w");

    fprintf(logger, "[START init_scheduler]\n");
    fprintf(logger, "[Type=%d]", sched_type);

    fclose(logger);
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    return 0;
}