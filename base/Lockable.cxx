// -*- c-file-style:  "bsd" -*-
#ifndef _LOCKABLE_CXX_
#define _LOCKABLE_CXX_

#include "Lockable.h"
#include <iostream>

template <int timeout>
long LockableBase<timeout>::msSinceLastLock()
{
    clock_t timeNow;
    struct tms spare;

    timeNow = times(&spare);

    // produces an inaccurate result, but it's better for the result
    // to be too small than for it to be large and negative...
    if (timeNow < m_lockTime) m_lockTime = 0;

    return ((timeNow - m_lockTime) * 1000L) / CLK_TCK;
}

template <int timeout>
void LockableBase<timeout>::setLockTimeToNow()
{
    struct tms spare;
    m_lockTime = times(&spare);
}

template <int timeout>
bool LockableBase<timeout>::timeoutExpired()
{
    if (timeout < 0) return false;

    if (msSinceLastLock() > timeout) {
	cout << "Timeout expired on Lockable " << this
	     << "(timeout is " << timeout << ", " << msSinceLastLock()
	     << " ms elapsed)" << endl;
	return true;
    } else {
	return false;
    }
}

template <int timeout>
void LockableBase<timeout>::refreshTimeout()
{
    if (!hasSomeLock()) {
        cerr << "Warning: attempt to refresh timeout on not-locked "
	     << "Lockable " << this << endl;
	return;
    }

    setLockTimeToNow();

    cout << "Refreshed lock on Lockable " << this << " at "
	 << m_lockTime << endl;
}


template <int timeout>
SimpleLockable<timeout>::~SimpleLockable()
{
    if (hasSomeLock()) {
	cerr << "Warning: deleting locked SimpleLockable " << this
	     << " (last locked " << msSinceLastLock() << "ms ago)" << endl;
    }
}

template <int timeout>
bool SimpleLockable<timeout>::l_lock()
{
    if (l_isLocked()) {
	cout << "SimpleLockable " << this << " already locked" << endl;
	return false;
    }
    m_isLocked = true;
    setLockTimeToNow();

    cout << "Have lock on SimpleLockable " << this << " at "
	 << getLockTime() << endl;

    return true;
}

template <int timeout>
void SimpleLockable<timeout>::l_unlock()
{
    if (!m_isLocked) {
        cerr << "Warning: attempt to unlock not-locked SimpleLockable " << this
	     << endl;
	return;
    }

    if (timeoutExpired()) {
	cerr << "Warning: unlocking lucky SimpleLockable " << this << " "
	     << msSinceLastLock() << "ms after lock\n(timeout is "
	     << timeout << ")" << endl;
    } else {
	cout << "Unlocking SimpleLockable " << this << endl;
    }

    m_isLocked = false;
}

template <int timeout>
bool SimpleLockable<timeout>::l_isLocked()
{
    if (!m_isLocked) return false;
    if (timeoutExpired()) m_isLocked = false;
    return m_isLocked;
}


template <int timeout>
MultiLockable<timeout>::~MultiLockable()
{
    if (hasSomeLock()) {
	cerr << "Warning: deleting locked MultiLockable " << this
	     << " (last locked " << msSinceLastLock() << "ms ago)" << endl;
    }
}

template <int timeout>
void MultiLockable<timeout>::lock()
{
    ++m_lockCount;
    setLockTimeToNow();

    cout << "Have lock on MultiLockable " << this << " at " << getLockTime()<< endl;
    cout << "(" << m_lockCount << " locks in total)" << endl;
}

template <int timeout>
void MultiLockable<timeout>::unlock()
{
    if (m_lockCount == 0) {
        cerr << "Warning: attempt to unlock not-locked MultiLockable " << this
	     << endl;
	return;
    }

    if (timeoutExpired()) {
	cerr << "Warning: unlocking lucky MultiLockable " << this << " "
	     << msSinceLastLock() << "ms after lock\n(timeout is "
	     << timeout << ")" << endl;
    } else {
	cout << "Unlocking MultiLockable " << this << " (" << m_lockCount-1
	     << " locks remain)" << endl;
    }

    --m_lockCount;
}

template <int timeout>
bool MultiLockable<timeout>::isLocked()
{
    if (m_lockCount == 0) return false;
    if (timeoutExpired()) m_lockCount = 0;
    return (m_lockCount != 0);
}


template <int timeout>
TransactionLockable<timeout>::TransactionNumber
TransactionLockable<timeout>::m_nextTransaction = 1;

template <int timeout>
TransactionLockable<timeout>::~TransactionLockable()
{
    if (hasSomeLock()) {
	cerr << "Warning: deleting locked TransactionLockable " << this
	     << " (last locked " << msSinceLastLock() << "ms ago)" << endl;
    }
}

template <int timeout>
bool TransactionLockable<timeout>::lock(TransactionNumber n)
{
    if (isLocked()) return false;
    m_transaction = n;
    setLockTimeToNow();

    cout << "Have lock on TransactionLockable " << this << " at "
	 << getLockTime() << " (transaction " << m_transaction
	 << ")" << endl;    

    return true;
}

template <int timeout>
void TransactionLockable<timeout>::unlock(TransactionNumber n)
{
    if (m_transaction != n) {
        cerr << "Warning: unauthorised transaction unlock!\n"
	     << "TransactionLockable " << this << " dealing with transaction "
	     << m_transaction << ", unlock request from " << n << endl;
	return;
    }

    if (timeoutExpired()) {
	cerr << "Warning: unlocking lucky TransactionLockable " << this << " "
	     << msSinceLastLock() << "ms after lock\n(timeout is "
	     << timeout << ")" << endl;
    } else {
	cout << "Unlocking TransactionLockable "
	     << this << " from transaction " << n << endl;
    }

    m_transaction = 0;
}

template <int timeout>
bool TransactionLockable<timeout>::isLocked()
{
    if (m_transaction == 0) return false;
    if (timeoutExpired()) m_transaction = 0;
    return (m_transaction != 0);
}

template <int timeout>
bool TransactionLockable<timeout>::isGood(TransactionNumber n)
{
    if (m_transaction == 0) {
	cout << "TransactionLockable::isGood(" << n << ") failed on " << this
	     << ":\nlockable is not currently locked" << endl;
	return false;
    }
    if (m_transaction != n) {
	cout << "TransactionLockable::isGood(" << n << ") failed on " << this
	     << ":\nlock belongs to transaction " << m_transaction << endl;
	return false;
    }
    refreshTimeout();
    cout << "TransactionLockable::isGood(" << n << ") succeeded on " << this
	 << endl;
    return true;
}

template <int timeout>
TransactionLockable<timeout>::TransactionNumber
TransactionLockable<timeout>::newTransactionNumber()
{
    return m_nextTransaction++;
}

#endif

