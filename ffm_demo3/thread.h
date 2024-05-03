#ifndef THREAD_H
#define THREAD_H

#include<thread>


class Thread
{
public:
    Thread(){};
    ~Thread(){
        if(t!=nullptr){
            this->Stop();
        }
    };
    int Start(){return 0;}
    int Stop(){
        abort=1;
        if(t!=nullptr){
            t->join();
            delete t;
        }
        return 0;
    }
    virtual void Run()=0;
protected:
    int abort=0;
    std::thread* t=nullptr;
};

#endif // THREAD_H
