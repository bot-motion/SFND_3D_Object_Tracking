#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <vector>

template<typename T> class RingBuffer {
	public:
		std::vector<T> m_vec;
		int m_head;
		int m_tail;
		int m_capacity;

	RingBuffer(int capacity)
	{
		m_vec.resize(capacity);
		m_data_size = 0;
		m_head = -1;
		m_tail = -1;
		m_capacity = capacity;
	}

	void push_back(T & item){
		m_data_size ++;
		if(m_data_size > m_capacity)
		{
			m_data_size = m_capacity;
		}
 
		if (m_tail == -1)
		{
			m_tail++;
			m_head++;
			m_vec[m_tail] = item;
		}
		else
		{
			m_tail = (m_tail + 1) % m_capacity;
			m_vec[m_tail] = item;
		
			if(m_tail == m_head)
			{
				m_head = (m_tail + 1) % m_capacity;
			}
		}
	}

	T *tail_prev(){
		// return the item right before the tail node
		int idx = (m_tail + m_capacity - 1) % m_capacity;
		return &(m_vec[idx]);
	}

	T* end()
	{
		// refers to the address just after the tail node
		if (m_tail == -1) return NULL;
		return &(m_vec[m_tail]) + 1;
	}

	int size()
	{
		return m_vec.size();
	}

	virtual ~RingBuffer(){

	}

	private:
		int m_data_size;
};

#endif 
