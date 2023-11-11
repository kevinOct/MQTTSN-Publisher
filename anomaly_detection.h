extern float STANDARD_DEVIATION;
extern float MEAN;
extern float SD_FACTOR;

// Function prototypes
int modify_params(float mean, float sd, float z_threshold);
void print_params(void);
bool detect_anomaly(float rn_value);
float get_sd(void);
float get_mean(void);
float get_z(void);

