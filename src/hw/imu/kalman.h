#ifndef G_KALMAN_H
#define G_KALMAN_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
/* Simple 1D Kalman filter (angle + gyro bias) - explicit struct */

struct kalman {
    float q_angle;    /* process noise variance for the angle */
    float q_bias;     /* process noise variance for the gyro bias */
    float r_measure;  /* measurement noise variance */

    float angle;      /* estimated angle (deg) */
    float bias;       /* estimated gyro bias (deg/s) */
    float rate;       /* last unbiased rate (deg/s) */

    /* error covariance matrix */
    float P00;
    float P01;
    float P10;
    float P11;
};


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/
/* Initialize kalman filter with tuning params */
void kalman_init(struct kalman *k, float q_angle, float q_bias, float r_measure);

/* Reset internal state to an initial angle (degrees) */
void kalman_reset(struct kalman *k, float angle);

/*
 * Update Kalman filter:
 * new_angle: measurement from accelerometer (deg)
 * new_rate : gyro rate (deg/s)
 * dt       : seconds
 * returns fused angle (deg)
 */
float kalman_update(struct kalman *k, float new_angle, float new_rate, float dt);

/**********************
 *      MACROS
 **********************/

#endif /*  G_KALMAN_H */
