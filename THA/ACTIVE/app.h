typedef struct _emp{
	char name[30];
	unsigned int age;
	char address[128];
	char designation [128];
} emp_t;

typedef struct _laptop_hw_t{
	float screen_size;
	float discount;
	unsigned int stock_qty;
	int warranty;
	int ram_size;
	int HDsize;
	char disk_type[8];
	int HDMI;
} laptop_hw_t;

typedef struct _laptop_t{
	char model[64];
	char company[64];
	unsigned int price;
	unsigned int yr;
	unsigned int serial_no;
	unsigned int no_acc;
	laptop_hw_t lap_hw;
} lap_t;

typedef struct _person{
	char name[30];
	unsigned int age;
	char address[128];
	char designation [128];
	emp_t *occupation;
	lap_t laptop;
} person_t;

typedef struct _engineer engineer_t;

typedef struct _manager{
	char manager_name[30];
	engineer_t *engineer;
} manager_t;

struct _engineer{
	char eng_name[30];
	manager_t *manager;
};



