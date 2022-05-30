#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <errno.h>

/* Defining a bool */
typedef int bool;
#define true 1
#define false 0

/* Implementing the queue as a linked list */

/* Each customer as a node */
struct node
{
	/* Customers unique identifier */
	int customerNo;
	/* Customers wait limit in the queue */
	int tolerance;
	/* Current time intervals the customer has waited */
	int currentWait;

	/* Next node in the list */
	struct node *next;
};
typedef struct node NODE;

/* Queue struct to hold the NODEs */
struct queue
{
	/* Number of nodes in the queue */
	int count;

	/* First node in the queue */
	NODE *front;
	/* Last node in the queue */
	NODE *rear;
};
typedef struct queue QUEUE;

/* server struct to represent each person working at the post office */
struct server
{
	/* Countdown of how long until free */
	int serviceTime;
	/* If free or not */
	bool active;
};
typedef struct server SERVER;

/*-------Function prototypes-------*/

void initialise(QUEUE *);
int isEmpty(QUEUE *);
void joinQueue(QUEUE *, int, int);
void deleteByKey(QUEUE *, int);

/*-------main function-------*/

int main(int argc, char **argv)
{
	/* If there are not enough arguments give an error message and exit the program */
	if (argc < 4)
	{
		fprintf(stderr, "Error insufficient inputs.\n"
						"usage: %s <filename>\n",
				argv[0]);
		return 1;
	}

	/* Create random number generator */
	const gsl_rng_type *T;
	gsl_rng *r;
	gsl_rng_env_setup();
	T = gsl_rng_default;
	r = gsl_rng_alloc(T);
	gsl_rng_set(r, time(0));

	/* Initialising variables used */
	/* Input file */
	FILE *ifp;
	/* Output file */
	FILE *ofp;
	/* Time the post office closes in time intervals */
	int closingTime;
	/* The current time interval of the simulation */
	int currentTime = 0;
	/* Maximum size of the queue */
	int maxSize;
	/* Current number in queue in the simulation */
	int numQueue = 0;
	/* Counting up to assign customerNo */
	int count = 0;
	/* Maximum number of servers */
	int numServers;
	/* Average time service takes */
	int avgServiceTime;
	int i;
	int j;
	int x;
	/* Number of servers currently busy */
	int busyCounters = 0;
	/* Customers who have been served and have left the post office */
	int fulfilled = 0;
	/* Customers who left before joining the queue as it was full */
	int unfulfilled = 0;
	/* Customers who left while in the queue as the wait exceeded their tolerance */
	int timedOut = 0;
	/* Extra time intervals the post office has stayed open to serve remaining customer after closing */
	int extraTime = 0;
	/* Average new customers per time interval */
	int avgNewCust;
	/* Average tolerance of customers */
	int avgTolerance;
	/* New customers this interval */
	int newCust;

	/* Creating a QUEUE, assigning memory to it and initialising it */
	QUEUE *q;
	if (!(q = malloc(sizeof(QUEUE))))
	{
		fprintf(stderr, "Out of memory\n", errno, strerror(errno));
		exit(1);
	}
	initialise(q);

	/* Removing any previous files with the same name as the output file */
	remove(argv[3]);

	/* Opening the input file for reading */
	ifp = fopen(argv[1], "r");

	/* Checking that the input file is not empty and giving an error if it is */
	if (ifp == NULL)
	{
		fprintf(stderr, "File not openable\n", errno, strerror(errno));
		return -1;
	}

	/* While not at the end of the file */
	while (!feof(ifp))
	{
		/* Reading the values from the file into the variables and if it is in the wrong format/ not enough values giving an error, closing the file and exiting */
		if (fscanf(ifp, "%*s %d,\n %*s %d,\n %*s %d,\n %*s %d,\n %*s %d,\n %*s %d,\n", &maxSize, &numServers, &closingTime, &avgServiceTime, &avgNewCust, &avgTolerance) != 6)
		{
			fprintf(stderr, "File format invalid\n");
			fclose(ifp);
			return -2;
		}
	}

	/* Closing the input file */
	fclose(ifp);

	/* Initialising an array of SERVER structs */
	SERVER cashiers[numServers];

	/* Assigning the initial values of each SERVER struct */
	for (i = 0; i < numServers; i++)
	{
		cashiers[i].serviceTime = avgServiceTime;
		cashiers[i].active = false;
	}

	/*-------------While still open-------------------*/
	while (currentTime < closingTime)
	{
		newCust = 0;
		/*-------While new customers are arriving-------*/
		/* Using poisson distribution with the average new customers */
		while (newCust < gsl_ran_poisson(r, avgNewCust))
		{
			/*-------If queue has space-------*/
			if (numQueue < maxSize)
			{
				/* Increment the number of people in the queue and add a NODE to the queue */
				count++;
				/* Using poisson distribution with the average tolerance */
				joinQueue(q, count, gsl_ran_poisson(r, avgTolerance));
				numQueue++;
				/* Increment ammount of new customers */
				newCust++;
			}

			/*------If queue has no space-------*/
			else
			{
				/* Increment unfulfilled and newCust */
				unfulfilled++;
				newCust++;
			}
		}

		/* Assigning a temp NODE to be equal to the first value of the QUEUE q */
		NODE *temp = q->front;

		/*------Check if each SERVER/cashier is active------*/
		for (x = 0; x < numServers; x++)
		{
			if (cashiers[x].active == true)
			{
				/* Decrement the time until free */
				cashiers[x].serviceTime--;
				/*-----If 0 time until free-----*/
				if (cashiers[x].serviceTime == 0)
				{
					/* Change active variable of SERVER struct to false, decrement busyCounters and increment fulfilled customers */
					cashiers[x].active = false;
					busyCounters--;
					fulfilled++;
				}
			}
		}

		/*------Iterate through each node------*/
		while (temp->next != NULL)
		{
			/*-----Iterate through each server-----*/
			for (j = 0; j < numServers; j++)
			{
				/*-----If the cashier is free and the current customer exists-----*/
				if (cashiers[j].active == false && temp->customerNo != 0)
				{
					/* Decrement number of customers in queue */
					numQueue--;
					/* Set the cashier to be active */
					cashiers[j].active = true;
					/* Set the service time */
					/* Using poisson distribution with the average tolerance */
					cashiers[j].serviceTime = gsl_ran_poisson(r, avgServiceTime);
					/* Delete the node from the queue */
					deleteByKey(q, temp->customerNo);
					/* Increment busyCounters */
					busyCounters++;
				}
			}
			/* Increment the customers wait */
			temp->currentWait++;
			/*-----If the customer has waited as long as their tolerance allows------*/
			if (temp->currentWait == temp->tolerance)
			{
				/* Delete the node from the queue */
				deleteByKey(q, temp->customerNo);
				/* Decrement the number in queue */
				numQueue--;
				/* Inecrement the number of customers who timed out */
				timedOut++;
				/* Point temp to the next node */
				temp = temp->next;
			}
			/*-----Otherwise-----*/
			else
			{
				/* Point temp to the next node */
				temp = temp->next;
			}
		}
		/*-------If temp is null the queue is empty------*/
		if (temp == NULL)
		{
			fprintf(stderr, "Queue NULL\n", errno, strerror(errno));
			;
		}

		/* Open the output file for appending */
		ofp = fopen(argv[3], "a+");
		/* Print into the file the information */
		fprintf(ofp, "Time interval: %d\n\
				Customers arrived: %d\n\
				Currently served: %d\n\
				In queue: %d\n\
				Customers fulfilled: %d\n\
				Customers unfulfilled: %d\n\
				Customers left the queue early: %d\n\n",
				currentTime, newCust, busyCounters, numQueue, fulfilled, unfulfilled, timedOut);
		/* Close the output file */
		fclose(ofp);

		/* Increment the current time */
		currentTime++;
	}
	/*-------------While closed but customers still in queue-------------------*/
	while (currentTime >= closingTime && extraTime < (closingTime / 5))
	{
		/* Assigning a temp NODE to be equal to the first value of the QUEUE q */
		NODE *temp = q->front;

		/*------Check if each SERVER/cashier is active------*/
		for (x = 0; x < numServers; x++)
		{
			if (cashiers[x].active == true)
			{
				/* Decrement the time until free */
				cashiers[x].serviceTime--;
				/*-----If 0 time until free-----*/
				if (cashiers[x].serviceTime == 0)
				{
					/* Change active variable of SERVER struct to false, decrement busyCounters and increment fulfilled customers */
					cashiers[x].active = false;
					busyCounters--;
					fulfilled++;
				}
			}
		}

		/*------Iterate through each node------*/
		while (temp->next != NULL)
		{
			/*-----Iterate through each server-----*/
			for (j = 0; j < numServers; j++)
			{
				/*-----If the cashier is free and the current customer exists-----*/
				if (cashiers[j].active == false && temp->customerNo != 0)
				{
					/* Decrement number of customers in queue */
					numQueue--;
					/* Set the cashier to be active */
					cashiers[j].active = true;
					/* Set the service time */
					/* Using poisson distribution with the average tolerance */
					cashiers[j].serviceTime = gsl_ran_poisson(r, avgServiceTime);
					/* Delete the node from the queue */
					deleteByKey(q, temp->customerNo);
					/* Increment busyCounters */
					busyCounters++;
				}
			}
			/* Increment the customers wait */
			temp->currentWait++;
			/*-----If the customer has waited as long as their tolerance allows-----*/
			if (temp->currentWait == temp->tolerance)
			{
				/* Delete the node from the queue */
				deleteByKey(q, temp->customerNo);
				/* Decrement the number in queue */
				numQueue--;
				/* Increment the number of customers who timed out */
				timedOut++;
				/* Point temp to the next node */
				temp = temp->next;
			}
			/*-----Otherwise-----*/
			else
			{
				/* Point temp to the next node */
				temp = temp->next;
			}
		}
		/*-------If temp is null the queue is empty-------*/
		if (temp == NULL)
		{
			fprintf(stderr, "File not openable\n", errno, strerror(errno));
			;
		}

		/* Open the output file for appending */
		ofp = fopen(argv[3], "a+");
		/* Print into the file the information */
		fprintf(ofp, "Time interval: %d\n\
				Currently served: %d\n\
				In queue: %d\n\
				Customers fulfilled: %d\n\
				Customers unfulfilled: %d\n\
				Customers left the queue early: %d\n\
				Extra time used: %d\n\n",
				currentTime, busyCounters, numQueue, fulfilled, unfulfilled, timedOut, extraTime);
		/* Close the output file */
		fclose(ofp);

		/* Increment the current time */
		currentTime++;
		/* Increment the extra time */
		extraTime++;
	}

	/* Free the QUEUE q and the rng */
	free(q);
	gsl_rng_free(r);
	return 0;
}

/*-------initialise function-------*/
/*Initialises a new empty QUEUE structure */
void initialise(QUEUE *q)
{
	/* Sets count to 0 */
	q->count = 0;
	/* Point front and rear to NULL */
	q->front = NULL;
	q->rear = NULL;
}

/*-------isEmpty function-------*/
/* Checks if the queue is empty or not by checking if the rear is NULL */
int isEmpty(QUEUE *q)
{
	return (q->rear == NULL);
}

/*-------joinQueue function-------*/
/* Adds a new value to the back of the QUEUE */
void joinQueue(QUEUE *q, int customerNo, int tolerance)
{
	/* Make a new temporary node */
	NODE *tmp;
	if (!(tmp = malloc(sizeof(NODE))))
	{
		fprintf(stderr, "Out of memory\n", errno, strerror(errno));
		exit(1);
	}
	/* Assign the values of the new node */
	tmp->customerNo = customerNo;
	tmp->tolerance = tolerance;
	tmp->currentWait = 0;
	/* Point to the next node as NULL as tmp is last in the queue */
	tmp->next = NULL;
	/*-----If the queue is not empty-----*/
	if (!isEmpty(q))
	{
		/* Point rear of queue to the new node */
		q->rear->next = tmp;
		/* Make the new node rear */
		q->rear = tmp;
	}
	/*-----If queue is empty-----*/
	else
	{
		/* Point front and rear of queue to the new node */
		q->front = q->rear = tmp;
	}
	/* Increment count */
	q->count++;
}

/*-------deleteByKey function-------*/
/* Delete a node from the queue by its key */
void deleteByKey(QUEUE *q, int key)
{
	/* Create 2 new nodes */
	NODE *prev, *cur;

	/*-----Iterate through the queue checking if customerNo matches key-----*/
	while (q->front != NULL && q->front->customerNo == key)
	{
		/* Set prev to the front node */
		prev = q->front;
		/* Set the front node to the next node */
		q->front = q->front->next;
		/* Free prev */
		free(prev);
		return;
	}

	/* Set prev to NULL */
	prev = NULL;
	/* Set cur to the front node */
	cur = q->front;

	/*-----Iterate through each node-----*/
	while (cur != NULL)
	{
		/*-----If customerNo of cur is equal to the key-----*/
		if (cur->customerNo == key)
		{
			/*-----If previous node is not NULL-----*/
			if (prev != NULL)
			{
				/* Set prev->next to cur->next */
				prev->next = cur->next;
			}
			/* Free cur */
			free(cur);
			return;
		}
		/* Set prev equal to cur */
		prev = cur;
		/* Set cur equal to cur->next */
		cur = cur->next;
	}
}
