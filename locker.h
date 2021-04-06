
/*
 *  线程同步机制包装类
 *  1.POSIX信号量
 *  2.互斥锁
 *  3.条件变量
 *  以下将其封装为3个类
 */
#ifndef LOCKER_H
#define LOCKER_H

#include <semaphore.h>
#include <exception>
#include <pthread.h>

/* 信号量机制的封装列 */
class sem
{
public:
    /* 创建并初始化信号量 */
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    /* 析构函数 销毁信号量 */
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    /* 等待信号量 */
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    /* 增加信号量 */
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

/* 互斥锁的封装类 */
class locker
{
public:
    /* 创建并初始化 */
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    /* 销毁互斥锁 */
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    /* 获取互斥锁 */
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    /* 释放互斥锁 */
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};
/* 条件变量的封装类 */
class cond
{
public:
    /* 创建并初始化 */
    cond()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    /* 销毁信号量和互斥锁 */
    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    /* 等待条件变量 */
    bool wait()
    {
        /* 条件变量要互斥访问，访问前先上锁 */
        int state = 0;
        pthread_mutex_lock(&m_mutex);
        state = pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_destroy(&m_mutex);
        return state == 0;
    }
    /* 唤醒等待条件变量的线程 */
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

private:
    /* 因为对条件变量的访问也要互斥,所以要增加一个互斥锁 */
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif