#ifndef _KIONIX_DEMO_VERSION_H
#define _KIONIX_DEMO_VERSION_H


enum kionix_driver_branches_e
{
    KD_ENUMERATED_BRANCHES_ENTRY_FIRST,
    KD_BRANCH_MAIN,
// zephyr-thread-analyzer-work-002
    KD_BRANCH_ZEPHYR_THREAD_ANALYZER_WORK_002,
    KD_BRANCH_ZEPHYR_THREAD_ANALYZER_WORK_003,
//    KD_BRANCH_

    KD_ENUMERATED_BRANCHES_ENTRY_LAST
};

#define KD_VERSION_NUMBER_MAJOR    1
#define KD_VERSION_NUMBER_MINOR    0
#define KD_VERSION_NUMBER_BRANCH   KD_BRANCH_ZEPHYR_THREAD_ANALYZER_WORK_002

#define KD_VERSION_STRING_LENGTH   (80)




#endif
