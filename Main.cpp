#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <stack>
#include <fstream>
#include <sys/wait.h>
#include <time.h>

#define MAX_DAY 2 // Have to change this

using namespace std;

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

enum WorkType
{
    OUTDOOR,
    INDOOR,
    OFFSITE
};

struct Task_Type
{
    int priority; // 1 = high, 2 = medium, 3 = low
    string description;
    int skillset;
    vector<int> RequiredResources;
    int WorkType;
};

vector<string> Skills = {"Electrician", "Plumber", "Carpenter", "Mason", "Laborer", "Engineer"};

vector<Task_Type> task_types = {
    {3, "Lighting Installation", ELECTRICIAN, {0, 0, 1}, INDOOR}, // Brick Cement Tool
    {3, "Plumbing Installation", PLUMBER, {0, 0, 1}, INDOOR},
    {3, "Wiring for New Construction", ELECTRICIAN, {0, 0, 1}, INDOOR},
    {3, "Construction Project Management", ENGINEER, {0, 0, 0}, OFFSITE},
    {3, "Project Planning and Design", ENGINEER, {0, 0, 0}, OFFSITE},

    {2, "Cement Mixing", LABORER, {0, 1, 0}, OUTDOOR},
    {2, "Brick Laying", MASON, {1, 1, 0}, OUTDOOR},
    {2, "Stone Wall Construction", MASON, {1, 1, 0}, OUTDOOR},
    {2, "Gas Line Installation", PLUMBER, {0, 0, 1}, INDOOR},
    {2, "Door and Window Installation", CARPENTER, {0, 1, 0}, INDOOR},

    {1, "Pipe Installation and Repair", PLUMBER, {0, 1, 1}, INDOOR},
    {1, "Pillar Construction", MASON, {1, 1, 0}, OUTDOOR},
    {1, "Staircase Installation", CARPENTER, {1, 1, 0}, INDOOR},
    {1, "Troubleshooting Electrical Issues", ELECTRICIAN, {0, 0, 1}, INDOOR},
    {1, "Concrete Repair", MASON, {0, 1, 0}, OUTDOOR}};

struct Worker
{
    int id;
    vector<int> skills;
    int SkillLevel;
    bool onBreak; // Flag indicating if the worker is on break
    int FatigueCounter;
    bool shift; // Flag indicating if the worker has even shift or odd shift

    Worker()
    {
    }
    Worker(const Worker &worker)
    {
        this->id = worker.id;
        this->skills = worker.skills;
        this->SkillLevel = worker.SkillLevel;
        this->onBreak = worker.onBreak;
        this->FatigueCounter = worker.FatigueCounter;
        this->shift = worker.shift;
    }
};

struct Task
{
    string name;
    int priority;
    string TaskDescription;
    int skillset;
    vector<int> RequiredResources;
    Worker *AssignedWorker;

    Task()
    {
    }

    Task(const Task &task)
    {
        this->name = task.name;
        this->priority = task.priority;
        this->TaskDescription = task.TaskDescription;
        this->skillset = task.skillset;
        this->RequiredResources = task.RequiredResources;
        if (task.AssignedWorker != nullptr)
        {
            this->AssignedWorker = new Worker(*task.AssignedWorker);
        }
        else
        {
            this->AssignedWorker = nullptr;
        }
    }

    void DisplayTask()
    {
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Task Name: " << name << endl;
        cout << "Task Priority: ";
        if (priority == 1)
        {
            cout << "High" << endl;
        }
        else if (priority == 2)
        {
            cout << "Medium" << endl;
        }
        else
        {
            cout << "Low" << endl;
        }
        cout << "Task Description: " << TaskDescription << endl;
        cout << "Task Skillset: " << Skills[skillset] << endl;
        if (AssignedWorker != nullptr)
        {
            cout << "Assigned Worker: " << AssignedWorker->id << endl;
        }
        cout << "Required Resources: " << endl;
        if (RequiredResources[0] > 0)
        {
            cout << "Bricks: " << RequiredResources[0] * 50 << endl;
        }
        if (RequiredResources[1] > 0)
        {
            cout << "Cement: " << RequiredResources[1] * 50 << endl;
        }
        if (RequiredResources[2] > 0)
        {
            cout << "Tools: " << RequiredResources[2] * 5 << endl;
        }
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    }
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
    ofstream file;

    void PrintLog()
    {
        file.open("log.txt");

        for (int i = 0; i < Day_Logs.size(); i++)
        {
            file << "Day " << Day_Logs[i].Day << "\n";
            file << "Weather: ";
            if (Day_Logs[i].weather == SUNNY)
            {
                file << "Sunny"
                     << "\n";
            }
            else if (Day_Logs[i].weather == RAINY)
            {
                file << "Rainy"
                     << "\n";
            }
            else if (Day_Logs[i].weather == NATURAL_DISASTER)
            {
                file << "Natural Disaster"
                     << "\n";
            }
            file << "Completed Tasks: "
                 << "\n";
            for (int j = 0; j < Day_Logs[i].CompletedTasks.size(); j++)
            {
                file << Day_Logs[i].CompletedTasks[j].name << "\n";
            }
            file << "Workers on Shift: "
                 << "\n";
            for (int j = 0; j < Day_Logs[i].WorkerOnShift.size(); j++)
            {
                file << "Worker " << Day_Logs[i].WorkerOnShift[j].id << "\n";
            }
            file << "\n";
        }

        file.close();
    }

} Log;

struct Resources
{
    int Bricks; // One Unit means 50 bricks
    int Cement; // One Unit means 50 bags of cement
    int Tools;  // One Unit means 5 tools

    void initialize()
    {
        Bricks = 3;
        Cement = 3;
        Tools = 3;
    }
};

sem_t Bricks; // One Unit means 50 bricks
sem_t Cement; // One Unit means 50 bags of cement
sem_t Tools;

Resources resources;

struct Resource_Utilization
{
    int Bricks = 0;
    int Cement = 0;
    int History_Bricks = 0;
    int History_Cement = 0;
    int History_Tools = 0;
} resource_utilization;

struct StateStack // State of the tasks that were interrupted
{
    stack<Task> State;
} TaskState;

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

// void CheckFatigue();
// void WorkerLRU();
// void StimulateBreak();

int main()
{
    srand(time(NULL));

    if(sem_init(&Bricks, 0, 3) == -1){
        perror("sem_init");
        exit(1);
    }
    if (sem_init(&Cement, 0, 3) == -1)
    {
        perror("sem_init");
        exit(1);
    }
    if (sem_init(&Tools, 0, 3) == -1)
    {
        perror("sem_init");
        exit(1);
    }
    resources.initialize();
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Starting Simulation" << endl;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl
         << endl;

    InitializeWorkers();
    while (true)
    {
        if (Log.Day > MAX_DAY && Memory.highPriorityQueue.empty() && Memory.mediumPriorityQueue.empty() && Memory.lowPriorityQueue.empty())
        {
            break;
        }
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Day " << Log.Day << " has started" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        ChangeShifts();
        Memory.currentWeather = CheckWeather();
        cout << "Weather: ";
        if (Memory.currentWeather == SUNNY)
        {
            cout << "Sunny" << endl;
        }
        else if (Memory.currentWeather == RAINY)
        {
            cout << "Rainy" << endl;
        }
        else if (Memory.currentWeather == NATURAL_DISASTER)
        {
            cout << "Natural Disaster" << endl;
        }
        ResourceReplishment();
        if (Log.Day <= MAX_DAY)
        {
            InitializeTasks();
        }
        // Worker Change based on fatigue, LRU, and break
        // DynamicTaskAdjustment();
        AssignTasks();
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Tasks for the Day" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        int i;
        for (int i = 0; i < Memory.IN_PROGRESS.size(); i++)
        {
            int *j = new int(i);
            pthread_t thread;
            Memory.CurrentTasks.push_back(thread);
            Task *task = new Task(Memory.IN_PROGRESS[*j]);
            pthread_create(&Memory.CurrentTasks[*j], NULL, TaskThread, (void *)task);
            usleep(1000000);
        }
        usleep(1000000);

        // Check if all tasks are completed
        void *status;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Day " << Log.Day << " has ended" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        for (int i = 0; i < Memory.CurrentTasks.size(); i++)
        {
            pthread_join(Memory.CurrentTasks[i], &status);
            //cout << *((int*)status) << endl;
            if (status == 0)
            {
                Memory.CompletedTasksForToday.push_back(Memory.IN_PROGRESS[i]);
                cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
                cout << Memory.IN_PROGRESS[i].TaskDescription << " has been completed" << endl;
                cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            }
            else if (*((int *)status) == 1)
            {
                cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
                cout << "Task " << Memory.IN_PROGRESS[i].TaskDescription << " is on hold due to lack of resources" << endl;
                cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            }
        }

        for (int i = 0; i < Memory.IN_PROGRESS.size(); i++)
        {
            Memory.IN_PROGRESS[i].AssignedWorker = nullptr;
        }
        for (int i = 0; i < Memory.OccupiedWorkers.size(); i++)
        {
            Memory.AvailableWorkers.push(Memory.OccupiedWorkers[i]);
        }
        // Fill log:
        Day_Log day_log;
        day_log.Day = Log.Day;
        day_log.weather = Memory.currentWeather;
        for (int i = 0; i < Memory.CompletedTasksForToday.size(); i++)
        {
            day_log.CompletedTasks.push_back(Memory.CompletedTasksForToday[i]);
        }
        while (!Memory.AvailableWorkers.empty())
        {
            day_log.WorkerOnShift.push_back(Memory.AvailableWorkers.front());
            Memory.NotONShiftWorkers.push_back(Memory.AvailableWorkers.front());
            Memory.AvailableWorkers.pop();
        }

        Log.Day_Logs.push_back(day_log);
        Memory.IN_PROGRESS.clear();
        Memory.CurrentTasks.clear();
        Memory.CompletedTasksForToday.clear();
        Memory.OccupiedWorkers.clear();
        Log.Day++;
    }
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Simulation has ended" << endl;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Writing Log" << endl;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    Log.PrintLog();

    sem_destroy(&Bricks);
    sem_destroy(&Cement);
    sem_destroy(&Tools);

    pthread_mutex_destroy(&dataMutex);
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&resourceMutex);

    pthread_exit(0);
}

void ResourceReplishment()
{
    int count = rand() % 2 + 1;
    if (Log.Day > 1)
    {
        cout << "Resource Replishment" << endl;
        cout << "Bricks Utilized the Day Before: " << resource_utilization.Bricks << endl;
        cout << "Cement Utilized the Day Before: " << resource_utilization.Cement << endl;
        cout <<"Current Cement: " << resources.Cement << endl;
        cout <<"Current Bricks: " << resources.Bricks << endl;
    }

    if (resource_utilization.Bricks > 0)
    {
        if (resource_utilization.History_Bricks / 2 > Log.Day && Log.Day > 10)
        {
            count = 1;
        }
        resources.Bricks += resource_utilization.Bricks;
        cout << "Bricks Replenished: " << resources.Bricks << endl;
    }
    if (resource_utilization.Cement > 0)
    {
        if (resource_utilization.History_Cement / 2 > Log.Day && Log.Day > 10)
        {
            count = 1;
        }
        resources.Cement += resource_utilization.Cement;
        cout << "Cement Replenished: " << resources.Cement << endl;
    }
    resource_utilization.Bricks = 0;
    resource_utilization.Cement = 0;
}

void InitializeTasks()
{
    int numTasks = rand() % 11 + 5;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Total Number of Tasks for the Day: " << numTasks << endl;
    cout << "Creating Tasks for the Day " << Log.Day << endl;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
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
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
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
    for (int i = 0; i < Memory.NotONShiftWorkers.size(); i++)
    {
        tempQueue.push_back(Memory.NotONShiftWorkers[i]);
    }
    Memory.NotONShiftWorkers.clear();
    while (!Memory.AvailableWorkers.empty())
    {
        tempQueue.push_back(Memory.AvailableWorkers.front());
        Memory.AvailableWorkers.pop();
    }

    for (int i = 0; i < tempQueue.size(); i++)
    {
        if (tempQueue[i].shift == shift)
        {
            Memory.AvailableWorkers.push(tempQueue[i]);
        }
        else
        {
            Memory.NotONShiftWorkers.push_back(tempQueue[i]);
        }
    }
}

void InitializeWorkers()
{
    int numWorkers = rand() % 11 + 10;
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Total Number of Workers: " << numWorkers << endl;

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
    cout << "Assigning Tasks" << endl;
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
                        break;
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
                task.AssignedWorker = new Worker(worker);
                // task.DisplayTask();
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size(); i++)
                {
                    if (tempQueue[i].id != worker.id)
                    {
                        Memory.AvailableWorkers.push(tempQueue[i]);
                    }
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
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
                        break;
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
                task.AssignedWorker = new Worker(worker);
                // task.DisplayTask();
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size(); i++)
                {
                    if (tempQueue[i].id != worker.id)
                    {
                        Memory.AvailableWorkers.push(tempQueue[i]);
                    }
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
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
                        break;
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
                task.AssignedWorker = new Worker(worker);
                // task.DisplayTask();
                Memory.OccupiedWorkers.push_back(worker);
                foundWorker = true;
                for (int i = 0; i < tempQueue.size(); i++)
                {
                    if (tempQueue[i].id != worker.id)
                    {
                        Memory.AvailableWorkers.push(tempQueue[i]);
                    }
                }
            }
            if (foundWorker == true)
            {
                Memory.IN_PROGRESS.push_back(task);
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
    return SUNNY;
}

void *TaskThread(void *task)
{ // 0 for success /1 for resource error /2 for Task change /10 for fork error /11 for pipe error
    Task *taskPtr = (Task *)task;
    int pipes[2];
    if (pipe(pipes) == -1)
    {
        perror("pipe");
        pthread_exit((void *)11);
    }
    int pid = fork();

    if (pid == -1)
    {
        perror("fork");
        pthread_exit((void *)10);
    }
    else if (pid == 0)
    {
        // check if resources are available:
        pthread_mutex_lock(&resourceMutex);
        cout << "Child Process:" << pthread_self() << endl;
        bool resourceError = false;
        bool check = false;
        bool check2 = false;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Checking Resources for " << taskPtr->TaskDescription << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        if (taskPtr->RequiredResources[0] == 1)
        {
            check = true;
            cout << "Bricks: " << resources.Bricks << endl;
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            if(resources.Bricks-count < 0){
                resourceError = true;
                cout << "Bricks Error" << endl;
            }
            else{
                resources.Bricks = resources.Bricks - count;
                cout << "Bricks Rem: " << resources.Bricks << endl;
            }
        }
        if (taskPtr->RequiredResources[1] == 1 && resourceError == false)
        {
            check2 = true;
            cout << "Cement: " << resources.Cement << endl;
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            if (resources.Cement - count < 0)
            {
                resourceError = true;
                cout << "Cement Error" << endl;
            }
            else
            {
                resources.Cement = resources.Cement - count;
                cout << "Cement Rem: " << resources.Cement << endl;
            }
            if (resourceError == true)
            {
                if (check == true)
                {
                    resources.Bricks += count; 
                    check = false;
                }
            }
        }
        if (taskPtr->RequiredResources[2] == 1 && resourceError == false)
        {
            cout << "Tools: " << resources.Tools << endl;
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            int i;
            if (resources.Tools - count < 0)
            {
                resourceError = true;
                cout << "Tools Error" << endl;
            }
            else
            {
                resources.Tools = resources.Tools - count;
                cout << "Tools Rem: " << resources.Tools << endl;
            }
            if (resourceError == true)
            {
                if (check == true)
                {
                    resources.Bricks += count;
                    check = false;
                }
                if (check2 == true)
                {
                    resources.Cement += count;
                    check2 = false;
                }
            }
        }
        cout << "Child Sent: " << resourceError << endl;
        // close(pipes[0]);
        write(pipes[1], &resourceError, sizeof(resourceError));
        pthread_mutex_unlock(&resourceMutex);
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process
        // wait(NULL);
        // cout << "Parent Process:" << pthread_self() << endl;
        bool resourceError;
        // close(pipes[1]);
        read(pipes[0], &resourceError, sizeof(resourceError));
        cout << "Parent recieved: " << resourceError << endl;
        if (resourceError == 1)
        {
            pthread_mutex_lock(&dataMutex);
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            cout << taskPtr->TaskDescription << " is on hold due to lack of resources" << endl;
            cout << taskPtr->TaskDescription << " is being put on hold" << endl;
            cout << taskPtr->TaskDescription << " is being put back in the queue" << endl;
            cout << "Worker " << taskPtr->AssignedWorker->id << " is being put back in the queue" << endl;
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            // int index = -1;
            // for (int i = 0; i < Memory.IN_PROGRESS.size(); i++)
            // {
            //     if (Memory.IN_PROGRESS[i].name == taskPtr->name)
            //     {
            //         index = i;
            //         break;
            //     }
            // }
            // Memory.AvailableWorkers.push(*taskPtr->AssignedWorker);
            // int index2 = -1;
            // for (int i = 0; i < Memory.OccupiedWorkers.size(); i++)
            // {
            //     if (Memory.OccupiedWorkers[i].id == taskPtr->AssignedWorker->id)
            //     {
            //         index2 = i;
            //         break;
            //     }
            // }
            // Memory.OccupiedWorkers.erase(Memory.OccupiedWorkers.begin() + index2);
            // taskPtr->AssignedWorker = nullptr;
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
            // Memory.IN_PROGRESS.erase(Memory.IN_PROGRESS.begin() + index);
            pthread_mutex_unlock(&dataMutex);
            pthread_exit((void *)1);
        }
        else
        {
            // if (taskPtr->priority == 1)
            // {
            //     int count = rand() % 30 + 1;
            //     if (count % 3 == 0)
            //     {
            //         taskPtr->AssignedWorker->FatigueCounter++;
            //     }
            // }
            pthread_mutex_lock(&dataMutex);
            int count = taskPtr->AssignedWorker->SkillLevel > 2 ? 1 : (taskPtr->AssignedWorker->SkillLevel > 1 ? 2 : 3);
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            cout << taskPtr->TaskDescription << " is being worked on by Worker " << taskPtr->AssignedWorker->id << endl;
            cout << taskPtr->TaskDescription << " is using the following resources: " << endl;
            // taskPtr->DisplayTask();
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
                resources.Tools += count;
                cout << "Task Has been Completed" << endl;
                cout << "Tools are now available" << endl;
            }
            else
            {
                cout << "Task Has been Completed" << endl;
            }
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            // usleep(100000);
            pthread_mutex_unlock(&dataMutex);
            pthread_exit(0);
        }
    }
    pthread_mutex_unlock(&dataMutex);
    pthread_exit(0);
}