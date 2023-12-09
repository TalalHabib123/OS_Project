#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <unistd.h>

using namespace std;

const int INITIAL_BRICKS = 50;
const int INITIAL_CEMENT = 30;
const int INITIAL_TOOLS = 10;

vector<vector<string>> tasks = {
    {"Urgent repairs", "foundation laying", "critical structural work"},
    {"General construction tasks", "bricklaying", "cement mixing"},
    {"Non-critical tasks", "finishing touches", "aesthetic elements"}
};

sem_t bricksSemaphore, cementSemaphore, toolsSemaphore;

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
struct Task
{
    string name;
    int priority;
    string TaskDescription;
    int AssignedWorker;
};
struct Worker
{
    int id;
    Task task;
    int SkillLevel;
    bool onBreak; // Flag indicating if the worker is on break
};
queue<Task> highPriorityQueue, mediumPriorityQueue, lowPriorityQueue;

queue<Worker> workersQueue;

int main()
{
    int numWorkers;
    cout << "Enter the number of workers: ";
    cin >> numWorkers;
    cout << endl;

    // Initialize semaphores
    sem_init(&bricksSemaphore, 0, INITIAL_BRICKS);
    sem_init(&cementSemaphore, 0, INITIAL_CEMENT);
    sem_init(&toolsSemaphore, 0, INITIAL_TOOLS);

    // Initialize workers
    for (int i = 0; i < numWorkers; i++)
    {
        Worker worker;
        worker.id = i;
        worker.SkillLevel = rand() % 3 + 1;
        worker.onBreak = false;
        workersQueue.push(worker);
    }

    // Initialize tasks
    int numTasks = rand() % 10 + 1;
    for (int i=0; i < numTasks; i++)
    {
        Task task;
        task.name = "Task " + to_string(i);
        task.priority = rand() % 3 + 1;
        task.TaskDescription = tasks[task.priority - 1][rand() % 3];
        task.AssignedWorker = -1;
        pthread_mutex_lock(&queueMutex);
        if (task.priority == 1)
            highPriorityQueue.push(task);
        else if (task.priority == 2)
            mediumPriorityQueue.push(task);
        else
            lowPriorityQueue.push(task);
        pthread_mutex_unlock(&queueMutex);
    }
}
