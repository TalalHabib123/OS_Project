#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <stack>

#define MAX_DAY 30

using namespace std;

struct Worker
{
    int id;
    vector<int> skills;
    int SkillLevel;
    bool onBreak; // Flag indicating if the worker is on break
    int FatigueCounter;
    bool shift; // Flag indicating if the worker has even shift or odd shift
};

struct Task
{
    string name;
    int priority;
    string TaskDescription;
    int skillset;
    vector<int> RequiredResources;
    Worker *AssignedWorker;
};

struct MemoryManagement
{
    queue<Task> highPriorityQueue, mediumPriorityQueue, lowPriorityQueue, OnHoldQueue;
    vector<Task> IN_PROGRESS;
    vector<pthread_t> CurrentTasks;

    vector<Task> CompletedTasksForToday;

    // On break means cannot work the rest of the day
    vector<Worker> OccupiedWorkers, OnBreakWorkers, NotONShiftWorkers;
    queue<Worker> AvailableWorkers;

    int currentWeather = SUNNY;

} Memory;

struct Day_Log
{
    vector<Task> CompletedTasks;
    int Day;
    int weather;
    vector<Worker> WorkerOnShift;
};

struct Log
{
    vector<Day_Log> Day_Logs;

    int Day = 1;
    int rainCounter = 0;
    int disasterCounter = 0;

} Log;

struct Resources
{
    sem_t Bricks; // One Unit means 50 bricks
    sem_t Cement; // One Unit means 50 bags of cement
    sem_t Tools;  // One Unit means 5 tools

    Resources()
    {
        sem_init(&Bricks, 0, 20);
        sem_init(&Cement, 0, 8);
        sem_init(&Tools, 0, 5);
    }
} resources;

struct Resource_Utilization
{
    int Bricks = 0;
    int Cement = 0;
    int History_Bricks = 0;
    int History_Cement = 0;
    int History_Tools = 0;
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
    vector<int> RequiredResources;
};

struct StateStack // State of the tasks that were interrupted
{
    stack<Task> State;
} TaskState;

vector<Task_Type> task_types = {
    {3, "Lighting Installation", ELECTRICIAN, {0, 0, 1}}, // Brick Cement Tool
    {3, "Plumbing Installation", PLUMBER, {0, 0, 1}},
    {3, "Wiring for New Construction", ELECTRICIAN, {0, 0, 1}},
    {3, "Construction Project Management", ENGINEER, {0, 0, 0}},
    {3, "Project Planning and Design", ENGINEER, {0, 0, 0}},

    {2, "Cement Mixing", LABORER, {0, 1, 0}},
    {2, "Brick Laying", MASON, {1, 1, 0}},
    {2, "Stone Wall Construction", MASON, {1, 1, 0}},
    {2, "Gas Line Installation", PLUMBER, {0, 0, 1}},
    {2, "Door and Window Installation", CARPENTER, {0, 1, 0}},

    {1, "Pipe Installation and Repair", PLUMBER, {0, 1, 1}},
    {1, "Pillar Construction", MASON, {1, 1, 0}},
    {1, "Staircase Installation", CARPENTER, {1, 1, 0}},
    {1, "Troubleshooting Electrical Issues", ELECTRICIAN, {0, 0, 1}},
    {1, "Concrete Repair", MASON, {0, 1, 0}}};

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t resourceMutex = PTHREAD_MUTEX_INITIALIZER;

void ResourceReplishment();
void InitializeTasks();
void InitializeWorkers();
void DynamicTaskAdjustment();
void AssignTasks();
int CheckWeather();
void ChangeShifts();
void *TaskThread(void *task);

int main()
{

    InitializeWorkers();

    while (true)
    {
        if (Log.Day > MAX_DAY && Memory.highPriorityQueue.empty() && Memory.mediumPriorityQueue.empty() && Memory.lowPriorityQueue.empty())
        {
            break;
        }
        ChangeShifts();
        Memory.currentWeather = CheckWeather();
        ResourceReplishment();
        InitializeTasks();

        // DynamicTaskAdjustment();
        AssignTasks();
        usleep(100000);

        // Check if all tasks are completed
        void *status;
        for (int i = 0; i < Memory.CurrentTasks.size(); i++)
        {
            pthread_join(Memory.CurrentTasks[i], &status);
            if (status == 0)
            {
                Memory.CompletedTasksForToday.push_back(Memory.IN_PROGRESS[i]);
                cout << "Task " << Memory.IN_PROGRESS[i].name << " has been completed" << endl;
                cout << "Worker " << Memory.IN_PROGRESS[i].AssignedWorker->id << " is being put back in the queue" << endl;
                Memory.AvailableWorkers.push(*Memory.IN_PROGRESS[i].AssignedWorker);
                Memory.IN_PROGRESS[i].AssignedWorker = nullptr;
            }
            else if (*(int *)status == 1)
            {
                cout << "Task " << Memory.IN_PROGRESS[i].name << " is on hold due to lack of resources" << endl;
            }
            // else if (*(int*)status == 2)
            // {

            // }
            // else if (*(int*)status == 10)
            // {

            // }
            // else if (*(int*)status == 11)
            // {

            // }
        }
        Memory.IN_PROGRESS.clear();
        Memory.CurrentTasks.clear();

        // Fill log:

        Log.Day++;
    }
}

void ResourceReplishment()
{
    if (resource_utilization.Bricks > 0)
    {
        for (int i = 0; i < resource_utilization.Bricks; i++)
        {
            sem_post(&resources.Bricks);
        }
    }
    if (resource_utilization.Cement > 0)
    {
        for (int i = 0; i < resource_utilization.Cement; i++)
        {
            sem_post(&resources.Cement);
        }
    }
}

void InitializeTasks()
{
    int numTasks = rand() % 11 + 5;

    for (int i = 0; i < numTasks; i++)
    {
        int TaskTypeIndex = rand() % task_types.size();
        Task task;
        task.name = "Task " + to_string(i);
        task.priority = task_types[TaskTypeIndex].priority;
        task.TaskDescription = task_types[TaskTypeIndex].description;
        task.skillset = task_types[TaskTypeIndex].skillset;
        task.RequiredResources = task_types[TaskTypeIndex].RequiredResources;
        task.AssignedWorker = nullptr;
        pthread_mutex_lock(&queueMutex);
        if (task.priority == 1)
            Memory.highPriorityQueue.push(task);
        else if (task.priority == 2)
            Memory.mediumPriorityQueue.push(task);
        else
            Memory.lowPriorityQueue.push(task);
        pthread_mutex_unlock(&queueMutex);
    }
}

void ChangeShifts()
{
    bool shift;
    if (Log.Day % 2 == 0)
    {
        shift = true;
    }
    else
    {
        shift = false;
    }
    vector<Worker> tempQueue;
    for(int i=0;i<Memory.NotONShiftWorkers.size();i++)
    {
        tempQueue.push_back(Memory.NotONShiftWorkers[i]);
    }
    Memory.NotONShiftWorkers.clear();
    while(!Memory.AvailableWorkers.empty())
    {
        tempQueue.push_back(Memory.AvailableWorkers.front());
        Memory.AvailableWorkers.pop();
    }

    for (int i = 0; i < tempQueue.size(); i++)
    {
        if (tempQueue[i].shift == shift)
        {
            Memory.NotONShiftWorkers.push_back(tempQueue[i]);
        }
        else
        {
            Memory.AvailableWorkers.push(tempQueue[i]);
        }
    }
}

void InitializeWorkers()
{
    int numWorkers = rand() % 11 + 5;
    for (int i = 0; i < numWorkers; i++)
    {
        Worker worker;
        worker.id = i;
        worker.SkillLevel = rand() % 3 + 1;
        int numSkills = rand() % 3 + 1;
        for (int j = 0; j < numSkills; j++)
        {
            int skill = rand() % 6;
            worker.skills.push_back(skill);
        }
        worker.onBreak = false;
        worker.FatigueCounter = 0;
        worker.shift = i % 2 == 0 ? true : false;
        Memory.AvailableWorkers.push(worker);
    }
}

void DynamicTaskAdjustment()
{
    // adjust based on weather, frequently used tasks, frequency used resources, tasks that frequently cause worker fatigue
    if (Memory.currentWeather == RAINY)
    {
    }
    else if (Memory.currentWeather == NATURAL_DISASTER)
    {
    }
    else if (Memory.currentWeather == SUNNY)
    {
    }
}

void AssignTasks()
{
    if (!Memory.highPriorityQueue.empty())
    {
        vector<Task> tempTasks;
        while (!Memory.highPriorityQueue.empty())
        {
            Task task = Memory.highPriorityQueue.front();
            Memory.highPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!Memory.AvailableWorkers.empty())
            {
                Worker worker = Memory.AvailableWorkers.front();
                Memory.AvailableWorkers.pop();
                bool hasSkill = false;
                for (int i = 0; i < worker.skills.size(); i++)
                {
                    if (worker.skills[i] == skillset)
                    {
                        tempQueue.push_back(worker);
                        hasSkill = true;
                    }
                }
                if (hasSkill == false)
                {
                    tempQueue2.push_back(worker);
                }
            }
            if (tempQueue2.size() > 0)
            {
                for (int i = 0; i < tempQueue2.size(); i++)
                {
                    Memory.AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    Memory.AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
                pthread_t thread;
                Memory.CurrentTasks.push_back(thread);
                pthread_create(&Memory.CurrentTasks[Memory.CurrentTasks.size() - 1], NULL, TaskThread, (void *)&task);
                usleep(100000);
            }
            else
            {
                tempTasks.push_back(task);
            }
        }
        if (tempTasks.size() > 0)
        {
            for (int i = 0; i < tempTasks.size(); i++)
            {
                Memory.highPriorityQueue.push(tempTasks[i]);
            }
        }
    }
    if (!Memory.mediumPriorityQueue.empty())
    {
        vector<Task> tempTasks;
        while (!Memory.mediumPriorityQueue.empty())
        {
            Task task = Memory.mediumPriorityQueue.front();
            Memory.mediumPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!Memory.AvailableWorkers.empty())
            {
                Worker worker = Memory.AvailableWorkers.front();
                Memory.AvailableWorkers.pop();
                bool hasSkill = false;
                for (int i = 0; i < worker.skills.size(); i++)
                {
                    if (worker.skills[i] == skillset)
                    {
                        tempQueue.push_back(worker);
                        hasSkill = true;
                    }
                }
                if (hasSkill == false)
                {
                    tempQueue2.push_back(worker);
                }
            }
            if (tempQueue2.size() > 0)
            {
                for (int i = 0; i < tempQueue2.size(); i++)
                {
                    Memory.AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    Memory.AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
                pthread_t thread;
                Memory.CurrentTasks.push_back(thread);
                pthread_create(&Memory.CurrentTasks[Memory.CurrentTasks.size() - 1], NULL, TaskThread, (void *)&task);
            }
            else
            {
                tempTasks.push_back(task);
            }
        }
        if (tempTasks.size() > 0)
        {
            for (int i = 0; i < tempTasks.size(); i++)
            {
                Memory.mediumPriorityQueue.push(tempTasks[i]);
            }
        }
    }
    if (!Memory.lowPriorityQueue.empty())
    {
        vector<Task> tempTasks;
        while (!Memory.lowPriorityQueue.empty())
        {
            Task task = Memory.lowPriorityQueue.front();
            Memory.lowPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!Memory.AvailableWorkers.empty())
            {
                Worker worker = Memory.AvailableWorkers.front();
                Memory.AvailableWorkers.pop();
                bool hasSkill = false;
                for (int i = 0; i < worker.skills.size(); i++)
                {
                    if (worker.skills[i] == skillset)
                    {
                        tempQueue.push_back(worker);
                        hasSkill = true;
                    }
                }
                if (hasSkill == false)
                {
                    tempQueue2.push_back(worker);
                }
            }
            if (tempQueue2.size() > 0)
            {
                for (int i = 0; i < tempQueue2.size(); i++)
                {
                    Memory.AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    Memory.AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
                pthread_t thread;
                Memory.CurrentTasks.push_back(thread);
                pthread_create(&Memory.CurrentTasks[Memory.CurrentTasks.size() - 1], NULL, TaskThread, (void *)&task);
            }
            else
            {
                tempTasks.push_back(task);
            }
        }
        if (tempTasks.size() > 0)
        {
            for (int i = 0; i < tempTasks.size(); i++)
            {
                Memory.lowPriorityQueue.push(tempTasks[i]);
            }
        }
    }
}

int CheckWeather()
{
    if (Log.rainCounter > 3 && Log.disasterCounter > 1)
    {
        return SUNNY;
    }
    else if (Log.rainCounter > 3 && Log.disasterCounter < 1)
    {
        return NATURAL_DISASTER;
        Log.disasterCounter++;
    }
    else if (Log.rainCounter < 3 && Log.disasterCounter > 1)
    {
        return RAINY;
        Log.rainCounter++;
    }
    else if (Log.rainCounter < 3 && Log.disasterCounter < 1)
    {
        int weather = rand() % 3;
        if (weather == 0)
        {
            return SUNNY;
        }
        else if (weather == 1)
        {
            return RAINY;
            Log.rainCounter++;
        }
        else if (weather == 2)
        {
            return NATURAL_DISASTER;
            Log.disasterCounter++;
        }
    }
}

void *TaskThread(void *task)
{ // 0 for success /1 for resource error /2 for Task change /10 for fork error /11 for pipe error
    Task *taskPtr = (Task *)task;
    int pipes[2];
    if (pipe(pipes) == -1)
    {
        pthread_exit((void *)11);
    }
    int pid = fork();

    if (pid == -1)
    {
        pthread_exit((void *)10);
    }
    else if (pid == 0)
    {
        // check if resources are available:
        pthread_mutex_lock(&resourceMutex);
        bool resourceError = false;
        bool check = false;
        bool check2 = false;
        if (taskPtr->RequiredResources[0] > 0)
        {
            check = true;
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            for (i = 0; i < count; i++)
            {
                if (sem_trywait(&resources.Bricks) == -1)
                {
                    resourceError = true;
                    break;
                }
            }
            if (resourceError == true)
            {
                for (int j = 0; j < i; j++)
                {
                    sem_post(&resources.Bricks);
                }
            }
        }
        if (taskPtr->RequiredResources[1] > 0 && resourceError == false)
        {
            check2 = true;
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            for (i = 0; i < count; i++)
            {
                if (sem_trywait(&resources.Cement) == -1)
                {
                    resourceError = true;
                    break;
                }
            }
            if (resourceError == true)
            {
                for (int j = 0; j < i; j++)
                {
                    sem_post(&resources.Cement);
                }
                if (check == true)
                {
                    for (int i = 0; i < count; i++)
                    {
                        sem_post(&resources.Bricks);
                    }
                    check = false;
                }
            }
        }
        if (taskPtr->RequiredResources[2] > 0 && resourceError == false)
        {
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            for (i = 0; i < count; i++)
            {
                if (sem_trywait(&resources.Tools) == -1)
                {
                    resourceError = true;
                    break;
                }
            }
            if (resourceError == true)
            {
                for (int j = 0; j < i; j++)
                {
                    sem_post(&resources.Tools);
                }
                if (check == true)
                {
                    for (int i = 0; i < count; i++)
                    {
                        sem_post(&resources.Bricks);
                    }
                    check = false;
                }
                if (check2 == true)
                {
                    for (int i = 0; i < count; i++)
                    {
                        sem_post(&resources.Cement);
                    }
                    check2 = false;
                }
            }
        }
        close(pipes[0]);
        pthread_mutex_unlock(&resourceMutex);
        write(pipes[1], &resourceError, sizeof(resourceError));
    }
    else
    {
        // Parent process
        bool resourceError;
        close(pipes[1]);
        read(pipes[0], &resourceError, sizeof(resourceError));
        if (resourceError == true)
        {
            cout << "Task " << taskPtr->name << " is on hold due to lack of resources" << endl;
            cout << "Task " << taskPtr->name << " is being put on hold" << endl;
            cout << "Task " << taskPtr->name << " is being put back in the queue" << endl;
            cout << "Worker " << taskPtr->AssignedWorker->id << " is being put back in the queue" << endl;
            pthread_mutex_lock(&dataMutex);
            int index = -1;
            for (int i = 0; i < Memory.IN_PROGRESS.size(); i++)
            {
                if (Memory.IN_PROGRESS[i].name == taskPtr->name)
                {
                    index = i;
                    break;
                }
            }
            Memory.AvailableWorkers.push(*taskPtr->AssignedWorker);
            int index2 = -1;
            for (int i = 0; i < Memory.OccupiedWorkers.size(); i++)
            {
                if (Memory.OccupiedWorkers[i].id == taskPtr->AssignedWorker->id)
                {
                    index2 = i;
                    break;
                }
            }
            Memory.OccupiedWorkers.erase(Memory.OccupiedWorkers.begin() + index2);
            taskPtr->AssignedWorker = nullptr;
            if (taskPtr->priority == 1)
            {
                Memory.highPriorityQueue.push(*taskPtr);
            }
            else if (taskPtr->priority == 2)
            {
                Memory.mediumPriorityQueue.push(*taskPtr);
            }
            else
            {
                Memory.lowPriorityQueue.push(*taskPtr);
            }
            // TaskState.State.push(*taskPtr);
            Memory.IN_PROGRESS.erase(Memory.IN_PROGRESS.begin() + index);
            pthread_mutex_unlock(&dataMutex);
            pthread_exit((void *)1);
        }
        else
        {
            pthread_mutex_lock(&dataMutex);
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            cout << "Task " << taskPtr->name << " is being worked on by Worker " << taskPtr->AssignedWorker->id << endl;
            if (taskPtr->RequiredResources[0])
            {
                resource_utilization.Bricks += count;
                resource_utilization.History_Bricks += count;
                cout << "Total Bricks Used By This Task: " << count * 50 << endl;
            }
            if (taskPtr->RequiredResources[1])
            {
                resource_utilization.Cement += count;
                resource_utilization.History_Cement += count;
                cout << "Total Bags of Cement Used By This Task: " << count * 50 << endl;
            }
            if (taskPtr->RequiredResources[2])
            {
                resource_utilization.History_Tools += count;
                cout << "Total Tools Used By This Task: " << count * 5 << endl;
                for (int i = 0; i < count; i++)
                {
                    sem_post(&resources.Tools);
                }
                cout << "Task Has been Completed" << endl;
                cout << "Tools are now available" << endl;
            }
            usleep(100000);
            pthread_mutex_unlock(&dataMutex);
            pthread_exit(0);
        }
    }
    pthread_exit(0);
}