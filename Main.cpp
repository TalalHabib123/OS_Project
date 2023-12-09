#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <unistd.h>

using namespace std;

struct Resources
{
    sem_t Bricks; // One Unit means 50 bricks
    sem_t Cement; // One Unit means 50 bags of cement
    sem_t Tools;  // One Unit means 20 tools

    Resources()
    {
        sem_init(&Bricks, 0, 20);
        sem_init(&Cement, 0, 8);
        sem_init(&Tools, 0, 5);
    }
} resources;

struct Resource_Utilization
{
    int Bricks=0;
    int Cement=0;
    int History_Bricks=0;
    int History_Cement=0;
} resource_utilization;

enum weather
{
    SUNNY,
    NATURAL_DISASTER,
    RAINY
};

enum Skillset
{
    ELECTRICIAN,
    PLUMBER,
    CARPENTER,
    MASON,
    LABORER,
    ENGINEER
};

struct Task_Type
{
    int priority; // 1 = high, 2 = medium, 3 = low
    string description;
    int skillset;
};

vector<Task_Type> task_types = {
    {3, "Lighting Installation", ELECTRICIAN},
    {3, "Wiring for New Construction", ELECTRICIAN},
    {3, "Stone Wall Construction", MASON},
    {3, "Construction Project Management", ENGINEER},
    {3, "Project Planning and Design", ENGINEER},

    {2, "Cement Mixing", LABORER},
    {2, "Brick Laying", MASON},
    {2, "Sewer Line Installation", PLUMBER},
    {2, "Gas Line Installation", PLUMBER},
    {2, "Paving Pathways", MASON},

    {1, "Pipe Installation and Repair", PLUMBER},
    {1, "Door and Window Installation", CARPENTER},
    {1, "Staircase Design and Installation", CARPENTER},
    {1, "Troubleshooting Electrical Issues", ELECTRICIAN},
    {1, "Concrete Repair", MASON}
};

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;

struct Worker
{
    int id;
    vector<int> skills;
    int SkillLevel;
    bool onBreak; // Flag indicating if the worker is on break
};

struct Task
{
    string name;
    int priority;
    string TaskDescription;
    Worker *AssignedWorker;
};

queue<Task> highPriorityQueue, mediumPriorityQueue, lowPriorityQueue, OnHoldQueue;

// On break means cannot work the rest of the day
vector<Worker> OccupiedWorkers, OnBreakWorkers;
queue<Worker> AvailableWorkers;

int main()
{
    int numWorkers=rand()%21+10;

    // Initialize workers
    for (int i = 0; i < numWorkers; i++)
    {
        Worker newWorker;
        newWorker.id = i;
        for (int i = 0; i < rand() % 4 + 1; i++)
        {
            newWorker.skills.push_back(rand() % 6);
        }
        newWorker.SkillLevel = rand() % 3 + 1;
        newWorker.onBreak = false;
        AvailableWorkers.push(newWorker);
    }

    // Initialize tasks
    
    while(true){
        
    }

    // int numTasks = rand() % 10 + 1;
    // for (int i = 0; i < numTasks; i++)
    // {
    //     Task task;
    //     task.name = "Task " + to_string(i);
    //     task.priority = rand() % 3 + 1;
    //     //task.TaskDescription = tasks[task.priority - 1][rand() % 3];
    //     task.AssignedWorker = nullptr;
    //     pthread_mutex_lock(&queueMutex);
    //     if (task.priority == 1)
    //         highPriorityQueue.push(task);
    //     else if (task.priority == 2)
    //         mediumPriorityQueue.push(task);
    //     else
    //         lowPriorityQueue.push(task);
    //     pthread_mutex_unlock(&queueMutex);
    // }
}
