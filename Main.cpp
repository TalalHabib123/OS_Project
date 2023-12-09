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
    int Bricks = 0;
    int Cement = 0;
    int History_Bricks = 0;
    int History_Cement = 0;
} resource_utilization;

enum weather
{
    SUNNY,
    NATURAL_DISASTER,
    RAINY
};

int currentWeather = SUNNY;

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
    int skillset;
    vector<int> RequiredResources;
    Worker *AssignedWorker;
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

queue<Task> highPriorityQueue, mediumPriorityQueue, lowPriorityQueue, OnHoldQueue;
vector<Task> IN_PROGRESS;
vector<pthread_t> CurrentTasks;

// On break means cannot work the rest of the day
vector<Worker> OccupiedWorkers, OnBreakWorkers;
queue<Worker> AvailableWorkers;

void ResourceReplishment();
void InitializeTasks();
void InitializeWorkers();
void DynamicTaskAdjustment();
void AssignTasks();
int CheckWeather();
void *TaskThread(void *task);

int main()
{
    int Day = 1;
    int rainCounter = 0;
    int disasterCounter = 0;
    InitializeWorkers();

    while (true)
    {
        if (Day > MAX_DAY && highPriorityQueue.empty() && mediumPriorityQueue.empty() && lowPriorityQueue.empty())
        {
            break;
        }
        currentWeather = CheckWeather(rainCounter, disasterCounter);
        ResourceReplishment();
        InitializeTasks();
        DynamicTaskAdjustment();
        AssignTasks();
        Day++;
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
            highPriorityQueue.push(task);
        else if (task.priority == 2)
            mediumPriorityQueue.push(task);
        else
            lowPriorityQueue.push(task);
        pthread_mutex_unlock(&queueMutex);
    }
}

void InitializeWorkers()
{
    int numWorkers = rand() % 11 + 5;
    for (int i = 0; i < numWorkers; i++)
    {
        Worker worker;
        worker.id = i;
        worker.SkillLevel = rand() % 5 + 1;
        int numSkills = rand() % 3 + 1;
        for (int j = 0; j < numSkills; j++)
        {
            int skill = rand() % 6;
            worker.skills.push_back(skill);
        }
        AvailableWorkers.push(worker);
    }
}

void DynamicTaskAdjustment()
{
    if (currentWeather == RAINY)
    {
    }
    else if (currentWeather == NATURAL_DISASTER)
    {
    }
    else if (currentWeather == SUNNY)
    {
    }
}

void AssignTasks()
{
    if (!highPriorityQueue.empty())
    {
        vector<Task> tempTasks;
        while (!highPriorityQueue.empty())
        {
            Task task = highPriorityQueue.front();
            highPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!AvailableWorkers.empty())
            {
                Worker worker = AvailableWorkers.front();
                AvailableWorkers.pop();
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
                    AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                IN_PROGRESS.push_back(task);
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
                highPriorityQueue.push(tempTasks[i]);
            }
        }
    }
    if (!mediumPriorityQueue.empty())
    {
        vector<Task> tempTasks;
        while (!mediumPriorityQueue.empty())
        {
            Task task = mediumPriorityQueue.front();
            mediumPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!AvailableWorkers.empty())
            {
                Worker worker = AvailableWorkers.front();
                AvailableWorkers.pop();
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
                    AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                IN_PROGRESS.push_back(task);
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
                mediumPriorityQueue.push(tempTasks[i]);
            }
        }
    }
    if (!lowPriorityQueue.empty())
    {
            vector<Task> tempTasks;
        while (!lowPriorityQueue.empty())
        {
            Task task = lowPriorityQueue.front();
            lowPriorityQueue.pop();
            int skillset = task.skillset;
            bool foundWorker = false;
            vector<Worker> tempQueue;
            vector<Worker> tempQueue2;
            while (!AvailableWorkers.empty())
            {
                Worker worker = AvailableWorkers.front();
                AvailableWorkers.pop();
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
                    AvailableWorkers.push(tempQueue2[i]);
                }
            }
            if (tempQueue.size() > 0)
            {
                sort(tempQueue.begin(), tempQueue.end(), [](const Worker &lhs, const Worker &rhs)
                     { return lhs.SkillLevel < rhs.SkillLevel; });
                Worker worker = tempQueue[tempQueue.size() - 1];
                task.AssignedWorker = &worker;
                OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size() - 1; i++)
                {
                    AvailableWorkers.push(tempQueue[i]);
                }
            }
            if (foundWorker == true)
            {
                IN_PROGRESS.push_back(task);
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
                lowPriorityQueue.push(tempTasks[i]);
            }
        }
    
    }
}

int CheckWeather(int &rainCounter, int &disasterCounter)
{
    if (rainCounter > 3 && disasterCounter > 1)
    {
        return SUNNY;
    }
    else if (rainCounter > 3 && disasterCounter < 1)
    {
        return NATURAL_DISASTER;
        disasterCounter++;
    }
    else if (rainCounter < 3 && disasterCounter > 1)
    {
        return RAINY;
        rainCounter++;
    }
    else if (rainCounter < 3 && disasterCounter < 1)
    {
        int weather = rand() % 3;
        if (weather == 0)
        {
            return SUNNY;
        }
        else if (weather == 1)
        {
            return RAINY;
            rainCounter++;
        }
        else if (weather == 2)
        {
            return NATURAL_DISASTER;
            disasterCounter++;
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
        bool resourceError = false;
        if (taskPtr->RequiredResources[0] > 0)
        {
            if (sem_trywait(&resources.Bricks) == -1)
            {
                resourceError = true;
            }
        }
        if (taskPtr->RequiredResources[1] > 0)
        {
            if (sem_trywait(&resources.Cement) == -1)
            {
                resourceError = true;
            }
        }
        if (taskPtr->RequiredResources[2] > 0)
        {
            if (sem_trywait(&resources.Tools) == -1)
            {
                resourceError = true;
            }
        }
        close(pipes[0]);
        write(pipes[1], &resourceError, sizeof(resourceError));
    }
    else
    {
        // Parent process

        // if (taskPtr->priority == 3)
        // {
        //     for (int i = 0; i < highPriorityQueue.size(); i++)
        //     {
        //         if (highPriorityQueue)
        //     }
        // }
        bool resourceError;
        close(pipes[1]);
        read(pipes[0], &resourceError, sizeof(resourceError));
        if (resourceError == true)
        {
            pthread_mutex_lock(&dataMutex);
            int index = -1;
            for (int i = 0; i < IN_PROGRESS.size(); i++)
            {
                if (IN_PROGRESS[i].name == taskPtr->name)
                {
                    index = i;
                    break;
                }
            }
            AvailableWorkers.push(*taskPtr->AssignedWorker);
            int index2 = -1;
            for (int i = 0; i < OccupiedWorkers.size(); i++)
            {
                if (OccupiedWorkers[i].id == taskPtr->AssignedWorker->id)
                {
                    index2 = i;
                    break;
                }
            }
            OccupiedWorkers.erase(OccupiedWorkers.begin() + index2);
            taskPtr->AssignedWorker = nullptr;
            if (taskPtr->priority == 1)
            {
                highPriorityQueue.push(*taskPtr);
            }
            else if (taskPtr->priority == 2)
            {
                mediumPriorityQueue.push(*taskPtr);
            }
            else
            {
                lowPriorityQueue.push(*taskPtr);
            }
            // TaskState.State.push(*taskPtr);
            IN_PROGRESS.erase(IN_PROGRESS.begin() + index);
            pthread_mutex_unlock(&dataMutex);
            pthread_exit((void *)1);
        }
        else
        {
            pthread_mutex_lock(&dataMutex);
            resource_utilization.Bricks += 1;
            resource_utilization.Cement += 1;
            resource_utilization.History_Bricks += 1;
            resource_utilization.History_Cement += 1;
            pthread_mutex_unlock(&dataMutex);
            pthread_exit(0);
        }
    }
    pthread_exit(0);
}