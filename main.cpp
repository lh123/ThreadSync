#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

const int MAX_COUNT = 20;//允许生产的最大数目
const int MAX_BUFFER = 10;//队列中的最大缓冲数目

int ticket = 0;//队列中的数目
int total_ticket = 0;//已生产的数目
bool is_need_quit = false;//控制生产者和消费者线程的退出

//队列的同步锁
std::mutex mLock;
std::condition_variable_any m_con_consumer;
std::condition_variable_any m_con_producer;

//控制Main线程的退出
int alive_count = 0;
std::mutex m_alive_count_lock;
std::condition_variable_any m_con_exit;

int main() {
    std::thread t([&] {//消费者
        m_alive_count_lock.lock();//锁住来安全的完成自加操作
        alive_count++;
        m_alive_count_lock.unlock();
        while (true) {
            mLock.lock();
            while (ticket <= 0 && is_need_quit == false) {//当前队列为空并且不需要退出
                m_con_consumer.wait(mLock);//等待队列不为空
            }
            if (is_need_quit && ticket <= 0) {//只有当队列为空并且需要退出时
                m_con_producer.notify_all();//唤醒生产者，防止生产者一直等待
                mLock.unlock();
                break;
            }
            ticket--;//消费掉一个元素
            std::cout << "Consumer" << ticket << std::endl;
            m_con_producer.notify_all();//通知生产者可以生产
            mLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        m_alive_count_lock.lock();//锁住来安全的完成自减操作
        alive_count--;
        m_con_exit.notify_all();//通知主线程自己准备退出
        m_alive_count_lock.unlock();
        std::cout << "Consumer Quit" << std::endl;
    });
    std::thread t1([&] {//生产者
        m_alive_count_lock.lock();//锁住来安全的完成自加操作
        alive_count++;
        m_alive_count_lock.unlock();
        while (true) {
            mLock.lock();
            while (ticket > MAX_BUFFER && is_need_quit == false) {//当前队列满了并且不需要退出那就等待
                m_con_producer.wait(mLock);
            }
            if (is_need_quit || total_ticket >= MAX_COUNT) {//需要退出或者达到生产总数
                is_need_quit = true;//设置退出标志位，方便消费者的退出
                m_con_consumer.notify_all();//唤醒消费者，防止其一直等待
                mLock.unlock();
                break;
            }
            ticket++;
            total_ticket++;
            m_con_consumer.notify_all();
            mLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Producer" << ticket << std::endl;
        }
        std::cout << "Producer Quit" << std::endl;
        m_alive_count_lock.lock();//锁住来安全的完成自减操作
        alive_count--;
        m_con_exit.notify_all();//通知主线程自己准备退出
        m_alive_count_lock.unlock();
    });
    t.detach();
    t1.detach();
    std::cin.get();//按下任意按键表示需要退出生产者和消费者
    m_alive_count_lock.lock();//获取锁，来安全的操作alive_count
    is_need_quit = true;//设置退出标志位
    while (alive_count != 0) {//仍然有线程在运行
        std::cout << "main wait" << std::endl;
        std::cv_status sta =
            m_con_exit.wait_for(m_alive_count_lock, std::chrono::seconds(5));//等待子线程退出，（5s超时）
        if (sta == std::cv_status::timeout) {//超时直接退出
            std::cout << "main time_out" << std::endl;
            break;
        }
    }
    m_alive_count_lock.unlock();
    std::cout << "main quit" << std::endl;
    std::cin.get();
    return 0;
}