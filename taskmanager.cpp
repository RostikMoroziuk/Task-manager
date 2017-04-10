#include "taskmanager.h"
#include "ui_taskmanager.h"

TaskManager::TaskManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TaskManager)
{
    ui->setupUi(this);
    timer = new QTimer();
    workerTimer = new QTimer();
    newProcess = new QProcess();
    appsVScrollPos = 0;
    procVScrollPos = 0;
    servicesVScrollPos = 0;
    appsHScrollPos = 0;
    procHScrollPos = 0;
    servicesHScrollPos = 0;
    //для визначення навантаження процесора
    updateTime = 3000;
    msInHour = 60.0*60.0*1000.0;
    msInMinute = 60.0*1000.0;
    msInSecond = 1000.0;

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(timeout()));
    connect(ui->applications, SIGNAL(clicked(QModelIndex)), this, SLOT(indexApp(QModelIndex)));
    connect(ui->processes, SIGNAL(clicked(QModelIndex)), this, SLOT(indexProc(QModelIndex)));
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    connect(ui->actionHigh, SIGNAL(triggered(bool)), this, SLOT(setUpdateSpeed()));
    connect(ui->actionMedium, SIGNAL(triggered(bool)), this, SLOT(setUpdateSpeed()));
    connect(ui->actionLow, SIGNAL(triggered(bool)), this, SLOT(setUpdateSpeed()));
    connect(ui->actionNew_task, SIGNAL(triggered(bool)), this, SLOT(on_newTask_clicked()));
    connect(ui->actionRefresh_now, SIGNAL(triggered(bool)), this, SLOT(timeout()));
    connect(workerTimer, SIGNAL(timeout()), this, SLOT(updateWorkTime()));

    apps = new QStandardItemModel();
    process = new QStandardItemModel();
    services = new QStandardItemModel();
    ui->applications->setHorizontalHeader(new QHeaderView(Qt::Horizontal));
    ui->processes->setHorizontalHeader(new QHeaderView(Qt::Horizontal));
    ui->services->setHorizontalHeader(new QHeaderView(Qt::Horizontal));

    ui->processes->setModel(process);
    ui->services->setModel(services);

    mainSystemInfo();
    activeApps();
    timer->start(3000);
}

TaskManager::~TaskManager()
{
    delete ui;
    delete apps;
    delete timer;
    delete workerTimer;
    delete process;
    delete services;
    if(newProcess)
        delete newProcess;
}

void TaskManager::timeout()//оновлює відкриту вкладку
{
    switch(ui->tabWidget->currentIndex())
    {
    case 0:
    {
        appsVScrollPos = ui->applications->verticalScrollBar()->value();
        appsHScrollPos = ui->applications->horizontalScrollBar()->value();
        QModelIndex curIndex = ui->applications->selectionModel()->currentIndex();//збереження поточно виділеного процеса, щоб виділити його при оновленні списку процесів
        if(curIndex.isValid())
            selectedActiveApps = apps->item(curIndex.row(), 0)->text();
        activeApps();
        timer->start(updateTime);
        workerTimer->stop();
        break;
    }
    case 1:
    {
        procVScrollPos = ui->processes->verticalScrollBar()->value();
        procHScrollPos = ui->processes->horizontalScrollBar()->value();
        QModelIndex curIndex = ui->processes->selectionModel()->currentIndex();
        if(curIndex.isValid())
            selectedActiveProces = process->item(curIndex.row(), 1)->text();
        activeProcess();
        timer->start(updateTime);
        workerTimer->stop();
        break;
    }
    case 2:
        workerTimer->start(1000);
        timer->start(updateTime);
        activeProcess();
        break;
    case 3:
    {
        servicesVScrollPos = ui->services->verticalScrollBar()->value();
        servicesHScrollPos = ui->services->horizontalScrollBar()->value();
        QModelIndex curIndex = ui->services->selectionModel()->currentIndex();
        if(curIndex.isValid())
            selectedActiveService = services->item(curIndex.row(), 1)->text();
        servicesList();
    }
    }
}

void TaskManager::setUpdateSpeed()
{
    timer->stop();
    QAction* sndr = (QAction*)sender();
    if(sndr==ui->actionHigh)
    {
        ui->actionHigh->setChecked(true);
        ui->actionLow->setChecked(false);
        ui->actionMedium->setChecked(false);
        timer->start(4000);
        updateTime = 4000;
    }
    else if(sndr==ui->actionLow)
    {
        ui->actionHigh->setChecked(false);
        ui->actionLow->setChecked(true);
        ui->actionMedium->setChecked(false);
        timer->start(2000);
        updateTime = 2000;
    }
    else if(sndr==ui->actionMedium)
    {
        ui->actionHigh->setChecked(false);
        ui->actionLow->setChecked(false);
        ui->actionMedium->setChecked(true);
        timer->start(3000);
        updateTime = 3000;
    }
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam )//завершення програми
{
    DWORD dwID ;

    GetWindowThreadProcessId(hwnd, &dwID) ;

    if(dwID == (DWORD)lParam)
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0) ;
    }

    return TRUE ;
}

void TaskManager::on_actionExit_triggered()
{
    exit(0);
}


//ВКЛАДКА ЗАПУЩЕНИХ ПРОГРАМ
QStandardItemModel* tempApps;// НЕ ЧІПАТИ!!!! ДИКИЙ КОСТИЛЬ!!!!
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM)//додаткова функція для знаходження активних аплікацій
{
    char Buff[255], NameOfClass[255];
    GetWindowText(hWnd, (LPWSTR)Buff, 254);
    GetClassName(hWnd, (LPWSTR)NameOfClass, 254);
    if(IsWindowVisible(hWnd))
    {
        QString str = QString::fromStdWString((LPWSTR)Buff);
        if(!str.isEmpty())
        {
            bool flag = true;
            DWORD dwID ;

            GetWindowThreadProcessId(hWnd, &dwID) ;

            for(int i = 0; i<tempApps->rowCount(); i++)//перевірка чи дане ід вже існує
            {
                if(tempApps->item(i, 1)->text().toULong() == dwID)
                {
                    flag = false;
                    break;
                }
            }
            if(flag)
                tempApps->appendRow(QList<QStandardItem*>()<<new QStandardItem(str)<<new QStandardItem(QString::number(dwID)));
        }
    }
    return TRUE;
}

void TaskManager::activeApps()
{
    apps->clear();
    apps->setHorizontalHeaderItem(0, new QStandardItem("App name"));
    apps->setHorizontalHeaderItem(1, new QStandardItem("Process ID"));
    tempApps = apps;//НАВІТЬ НЕ ПРОБУЙ ЦЕ МІНЯТИ

    EnumWindows((WNDENUMPROC)EnumWindowsProc, 0);
    ui->applications->setModel(apps);
    ui->applications->resizeColumnsToContents();

    if(!selectedActiveApps.isEmpty())
    {
        for(int i=0;i<apps->rowCount();i++)
        {
            if(selectedActiveApps == apps->item(i, 0)->text())
            {
                ui->applications->selectRow(i);
                break;
            }
        }
    }
    else
        ui->applications->selectRow(0);

    ui->applications->verticalScrollBar()->setValue(appsVScrollPos);
    ui->applications->horizontalScrollBar()->setValue(appsHScrollPos);
}

void TaskManager::indexApp(QModelIndex index)
{
    if(index.isValid())
        ui->endTask->setEnabled(true);
    else
        ui->endTask->setEnabled(false);
}

void TaskManager::on_endTask_clicked()//закриття вікна
{
    QModelIndex mi = ui->applications->selectionModel()->currentIndex();
    if(mi.isValid())
    {
        EnumWindows((WNDENUMPROC)TerminateAppEnum, (DWORD)apps->item(mi.row(), 1)->text().toULong());
    }
    else
    {
        QMessageBox::warning(this, "Error", "Has not select item");
    }

}

void TaskManager::on_newTask_clicked()
{
    delete newProcess;
    newProcess = new QProcess();
    newTaskFileName = QFileDialog::getOpenFileName(this, "New task", "*.exe");
    qDebug()<<newTaskFileName;
    if(!newTaskFileName.isEmpty())
    {
        for(int i =  0 ; i < newTaskFileName.length();++i)//зміна шляху (шоб був коректний шлях)
        {
            if(newTaskFileName.at(i)=='/')
            {
                newTaskFileName.remove(i,1);
                newTaskFileName.insert(i,"\\");
            }
        }
        qDebug()<<newTaskFileName;
        newProcess->start("cmd /k " + newTaskFileName);
    }
}


//ВКЛАДАКА ПРОЦЕСІВ!!!!!
void TaskManager::activeProcess()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//знімок запущених процесів
    PROCESSENTRY32 pe;    //інформація про знайдений процес

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        QMessageBox::warning(this, "Find process error", "Can not take snaphot\nINVALID_HANDLE_VALUE");
        return;
    }
    process->clear();
    process->setHorizontalHeaderItem(0, new QStandardItem("Process name"));
    process->setHorizontalHeaderItem(1, new QStandardItem("Process ID"));
    process->setHorizontalHeaderItem(2, new QStandardItem("Threads number"));
    process->setHorizontalHeaderItem(3, new QStandardItem("Parent PID"));
    process->setHorizontalHeaderItem(4, new QStandardItem("Priority"));
    process->setHorizontalHeaderItem(5, new QStandardItem("CPU"));
    process->setHorizontalHeaderItem(6, new QStandardItem("Memory"));

    pe.dwSize = sizeof(PROCESSENTRY32);
    int procCount = 0;
    int threadCount = 0;
    cpu = 0.0;

    if (Process32First(hSnapshot, &pe))	//Пошук першого знімку в снепшоті
    {
        do
        {
            procCount++;
            //if (!pe.th32ProcessID) continue;	// Пропуск [System process]

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,FALSE, pe.th32ProcessID);	//Отримання рівня доступу управління процесом
            //визначення пріоритета процеса
            QString procPriority;
            switch (GetPriorityClass(hProcess))	//Получаем пріоритет процеса
            {
            case HIGH_PRIORITY_CLASS:
                procPriority = "High";
                break;
            case IDLE_PRIORITY_CLASS:
                procPriority = "Idle";
                break;
            case NORMAL_PRIORITY_CLASS:
                procPriority = "Normal";
                break;
            case REALTIME_PRIORITY_CLASS:
                procPriority = "Realtime";
                break;
            case ABOVE_NORMAL_PRIORITY_CLASS:
                procPriority = "Above normal";
                break;
            case BELOW_NORMAL_PRIORITY_CLASS:
                procPriority = "Below normal";
                break;
            default:
                procPriority = "No access";
            }

            //визначення загрузки процесора
            FILETIME ftCreation,ftExit,ftKernel,ftUser;
            SYSTEMTIME stCreation,stExit,stKernel,stUser;
            double procent=0;//відсоток завантаження процесора
            if(GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser))//час виділений процесу системою
            {
                //приведення часу в нормальний формат
                FileTimeToSystemTime(&ftCreation, &stCreation);
                FileTimeToSystemTime(&ftExit, &stExit);
                FileTimeToSystemTime(&ftUser, &stUser);
                FileTimeToSystemTime(&ftKernel, &stKernel);
                //час роботи в режимі користувача
                double user = ((double)stUser.wHour*msInHour +
                               (double)stUser.wMinute*msInMinute +
                               (double)stUser.wSecond*msInSecond +
                               (double)stUser.wMilliseconds)/updateTime;//час в інтервалі якого знято показники
                //час роботи в режимі ядра
                double kernel = ((double)stKernel.wHour*msInHour +
                                 (double)stKernel.wMinute*msInMinute +
                                 (double)stKernel.wSecond*msInSecond +
                                 (double)stKernel.wMilliseconds)/updateTime;//час в інтервалі якого знято показники
                double res = user + kernel;



                QHash<DWORD, double>::iterator it = timeForProces.find(pe.th32ProcessID);
                if(it!=timeForProces.end())//якшо такий елемент знайдено
                {
                    procent = (res - it.value())*100.0;
                    timeForProces.remove(pe.th32ProcessID);
                }
                timeForProces.insert(pe.th32ProcessID, res);
                cpu+=procent;
            }

            //визначення пам'яті процеса
            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
            process->appendRow(QList<QStandardItem*>()<<new QStandardItem(QString::fromStdWString(pe.szExeFile))
                               <<new QStandardItem(QString::number(pe.th32ProcessID))
                               <<new QStandardItem(QString::number(pe.cntThreads))
                               <<new QStandardItem(QString::number(pe.th32ParentProcessID))
                               <<new QStandardItem(procPriority));
            if(procPriority=="No access")
            {
                process->setItem(process->rowCount()-1, 5, new QStandardItem("No access"));
                process->setItem(process->rowCount()-1, 6, new QStandardItem("No access"));
            }
            else
            {
                process->setItem(process->rowCount()-1, 5, new QStandardItem(QString::number(procent, 'g', 2)+"%"));
                process->setItem(process->rowCount()-1, 6, new QStandardItem(QString::number(((double)pmc.WorkingSetSize/totalMemorySize)*100, 'g', 2)+"%"));
            }

            threadCount+=pe.cntThreads;
            CloseHandle(hProcess);
        } while (Process32Next(hSnapshot, &pe));	//пошук наступного процеса
    }
    else
        QMessageBox::warning(this, "Show process error", "Cannot showproceses");

    CloseHandle(hSnapshot);

    if(!selectedActiveProces.isEmpty())//якшо закинути вище, не буде працювати виділення через постійне оновлення моделі
    {
        for(int i=0;i<process->rowCount();i++)
        {
            if(selectedActiveProces == process->item(i, 1)->text())
            {
                ui->processes->selectRow(i);
                break;
            }
        }
    }
    else
        ui->processes->selectRow(0);

    ui->processes->setModel(process);
    ui->processes->verticalScrollBar()->setValue(procVScrollPos);
    ui->processes->horizontalScrollBar()->setValue(procHScrollPos);
    ui->procNum->setText(QString::number(procCount));
    ui->threadsNum->setText(QString::number(threadCount));
    ui->cpuNum->setText(QString::number(cpu, 'g', 2)+"%");
    ui->processes->resizeColumnsToContents();

    ui->cpuPerf->setValue(cpu);
}

void TaskManager::indexProc(QModelIndex index)
{
    if(index.isValid())
        ui->endTask2->setEnabled(true);
    else
        ui->endTask2->setEnabled(false);
}

void TaskManager::on_endTask2_clicked()
{
    QModelIndex mi = ui->processes->selectionModel()->currentIndex();
    if(mi.isValid())
    {
        EnumWindows((WNDENUMPROC)TerminateAppEnum, (DWORD)process->item(mi.row(), 1)->text().toULong());
    }
    else
    {
        QMessageBox::warning(this, "Error", "Has not select item");
    }
}


//ВКЛАДКА PERFOMANCE
void TaskManager::mainSystemInfo()
{
    //ім'я системи
    char buf[20];
    DWORD size = 20;
    GetUserNameA((LPSTR)buf, &size);
    ui->userName->setText(QString(buf));
    //час роботи системи
    workerTimer->start(1000);
    updateWorkTime();
    //розмір пам'яті
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    totalMemorySize = statex.ullTotalPhys;
    ui->physMemory->setText(QString::number(totalMemorySize/(1024*1024), 'g', 6)+"MB");//фізична пам'ять
}

void TaskManager::updateWorkTime()
{
    QTime systemWorkTime(0,0,0);
    systemWorkTime = systemWorkTime.addMSecs(GetTickCount());
    ui->uptime->setText(systemWorkTime.toString("hh:mm:ss"));

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    ui->memUse->setValue(statex.dwMemoryLoad);
    ui->virtMemmory->setText(QString::number(statex.ullTotalVirtual/(1024*1024), 'g', 6) + "MB");//віртуальна пам'ять об'єм
}


//СПИОК СЕРВІСІВ
void TaskManager::servicesList()
{
    services->clear();
    services->setHorizontalHeaderItem(0, new QStandardItem("Name"));
    services->setHorizontalHeaderItem(1, new QStandardItem("Services name"));
    services->setHorizontalHeaderItem(2, new QStandardItem("Type"));
    services->setHorizontalHeaderItem(3, new QStandardItem("State"));
    services->setHorizontalHeaderItem(4, new QStandardItem("PID"));

    unsigned char buf[256000];
    DWORD bufSize = 256000;
    DWORD bufSizeNeed=0, serviceNumber=0;
    SC_HANDLE hSCM = NULL;
    LPENUM_SERVICE_STATUS_PROCESS info = NULL;//структури сервісів

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);//менеджер доступу до сервісів (повний доступ)
    if(hSCM==NULL)
    {
        if(GetLastError()==ERROR_ACCESS_DENIED)
            QMessageBox::warning(this, "Acces error", "The requested access was denied.");
        else if(GetLastError()==ERROR_DATABASE_DOES_NOT_EXIST)
            QMessageBox::warning(this, "Acces error", "The specified database does not exist.");
        timer->stop();
        return;
    }

    EnumServicesStatusEx(hSCM,
            SC_ENUM_PROCESS_INFO,//беремо інформацію про процеси
            SERVICE_WIN32 | SERVICE_DRIVER | SERVICE_FILE_SYSTEM_DRIVER | SERVICE_KERNEL_DRIVER, // пошук типу сервісів
            SERVICE_STATE_ALL,//Всі сервіси і активні і неактивні
            buf,//буфер списку сервісів
            bufSize,
            &bufSizeNeed,//вертає кількість необхідних байт, якшо замало пам'яті в буфері
            &serviceNumber,//кількість сервісів
            NULL,
            NULL);
    qDebug()<<bufSizeNeed;
    qDebug()<<serviceNumber;

    info = (LPENUM_SERVICE_STATUS_PROCESS)buf;
    int activeServices=0;
    for(ULONG i = 0;i<serviceNumber;i++)
    {
        //визначення типу
        QString type;
        switch(info[i].ServiceStatusProcess.dwServiceType)
        {
        case SERVICE_FILE_SYSTEM_DRIVER:
            type = "File system driver";
            break;
        case SERVICE_KERNEL_DRIVER:
            type = "Kernel driver";
            break;
        case SERVICE_WIN32_OWN_PROCESS:
            type = "Own process";
            break;
        case SERVICE_WIN32_SHARE_PROCESS:
            type = "Shares a process";
            break;
        }
        //визначення стану
        QString state;
        switch(info[i].ServiceStatusProcess.dwCurrentState)
        {
        case SERVICE_PAUSED:
            state = "Paused";
            break;
        case SERVICE_RUNNING:
            state = "Running";
            activeServices++;
            break;
        case SERVICE_STOPPED:
            state = "Stopped";
            break;
        }

        services->appendRow(QList<QStandardItem*>()<< new QStandardItem(QString::fromStdWString(info[i].lpDisplayName))
                            << new QStandardItem(QString::fromStdWString(info[i].lpServiceName))
                            << new QStandardItem(type)
                            << new QStandardItem(state)
                            << new QStandardItem(QString::number(info[i].ServiceStatusProcess.dwProcessId)));
    }
    if(!selectedActiveService.isEmpty())
        for(int i=0;i<services->rowCount();i++)
        {
            if(selectedActiveService == services->item(i, 1)->text())
            {
                ui->services->selectRow(i);
                break;
            }
        }
    else
        ui->services->selectRow(0);

    ui->services->setModel(services);
    ui->services->verticalScrollBar()->setValue(servicesVScrollPos);
    ui->services->horizontalScrollBar()->setValue(servicesHScrollPos);
    ui->servicesNum->setText(QString::number(serviceNumber));
    ui->activeServicesNum->setText(QString::number(activeServices));
    ui->services->resizeColumnsToContents();

    CloseServiceHandle(hSCM);
}

void TaskManager::on_actionAuthor_triggered()
{
    QMessageBox::about(this, "Author", "TaskManager v1.0\nCopyright© Morozyuk Rostyslav\nLviv 2017");
}
