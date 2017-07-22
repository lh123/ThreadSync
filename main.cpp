#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

const int MAX_COUNT = 20;
const int MAX_BUFFER = 10;

int ticket = 0;
int total_ticket = 0;
bool is_need_quit = false;
std::mutex mLock;
std::condition_variable_any m_con_consumer;
std::condition_variable_any m_con_producer;

int alive_count = 0;
std::mutex m_alive_count_lock;
std::condition_variable_any m_con_exit;

int main() {
    std::thread t([&] {
        m_alive_count_lock.lock();
        alive_count++;
        m_alive_count_lock.unlock();
        while (true) {
            mLock.lock();
            while (ticket <= 0 && is_need_quit == false) {
                m_con_consumer.wait(mLock);
            }
            if (is_need_quit && ticket <= 0) {
                m_con_producer.notify_all();
                mLock.unlock();
                break;
            }
            ticket--;
            std::cout << "Consumer" << ticket << std::endl;
            m_con_producer.notify_all();
            mLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        m_alive_count_lock.lock();
        alive_count--;
        m_con_exit.notify_all();
        m_alive_count_lock.unlock();
        std::cout << "Consumer Quit" << std::endl;
    });
    std::thread t1([&] {
        m_alive_count_lock.lock();
        alive_count++;
        m_alive_count_lock.unlock();
        while (true) {
            mLock.lock();
            while (ticket > MAX_BUFFER && is_need_quit == false) {
                m_con_producer.wait(mLock);
            }
            if (is_need_quit || total_ticket >= MAX_COUNT) {
                is_need_quit = true;
                m_con_consumer.notify_all();
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
        m_alive_count_lock.lock();
        alive_count--;
        m_con_exit.notify_all();
        m_alive_count_lock.unlock();
    });
    t.detach();
    t1.detach();
    std::cin.get();
    m_alive_count_lock.lock();
    is_need_quit = true;
    while (alive_count != 0) {
        std::cout << "main wait" << std::endl;
        std::cv_status sta =
            m_con_exit.wait_for(m_alive_count_lock, std::chrono::seconds(5));
        if (sta == std::cv_status::timeout) {
            std::cout << "main time_out" << std::endl;
            break;
        }
    }
    m_alive_count_lock.unlock();
    std::cout << "main quit" << std::endl;
    std::cin.get();
    return 0;
}