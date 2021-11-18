#ifndef _KD_DEVELOPMENT_FLAGS
#define _KD_DEVELOPMENT_FLAGS


//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

// --- DEVELOPMENT FLAGS BEGIN ---

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL

// KX132-1211 Configuration:
#define DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS (1)
#define DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE         (0)

// KX132-1211 Readings:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID           (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID                   (1)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ        (1)


#define NN_DEV__ENABLE_INT_MAIN_TESTS                     (0)
#define NN_DEV__ENABLE_THREAD_IIS2DH_SENSOR               (1)
#define NN_DEV__ENABLE_THREAD_LIS2DH_SENSOR               (0)
#define NN_DEV__ENABLE_THREAD_SIMPLE_CLI                  (1)

#define NN_DEV__ENABLE_IIS2DH_TEMPERATURE_READGINGS       (0)

#define NN_DEV__TEST_SCOREBOARD_GLOBAL_SETTING            (1)


// 2021-11-17 -
#define KD_DEV__ENABLE_IIS2DH_TEMPERATURE_READINGS        (0)
#define KD_DEV__SET_BDU_BEFORE_TEMP_READING_THEN_UNSET    (1)


// --- DEVELOPMENT FLAGS END ---



#endif // _KD_DEVELOPMENT_FLAGS
