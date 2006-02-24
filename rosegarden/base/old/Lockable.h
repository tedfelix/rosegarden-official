// -*- c-basic-offset: 4 -*-

// -*- c-file-style:  "bsd" -*-
#ifndef _LOCKABLE_H_
#define _LOCKABLE_H_

#include <sys/times.h>

/** LockableBase implements a lock timer.  It doesn't implement any
    locking semantics itself, that's up to the subclass

    Timeouts are in milliseconds.  The defaults are quite aggressive,
    but a client will only lose a lock if the timeout expires *and*
    another client requests the lock; a Lockable won't go out of its
    way to reap stale locks.  So on something like an Element that's
    generally acquired, used and abandoned, the default might be okay.
    
    A Lockable with negative timeout will never time out.  (A Lockable
    with zero timeout will always time out.)
*/

template <int timeout>
class LockableBase
{
public:
    virtual ~LockableBase() { }

    /** this is ok public: clients that haven't got the lock can only
	harm themselves by refreshing the timeout */
    virtual void refreshTimeout();

protected:
    LockableBase() : m_lockTime(0) { }

    virtual long msSinceLastLock();
    virtual void setLockTimeToNow();
    virtual bool timeoutExpired();
    virtual bool hasSomeLock() = 0;

    /// should only be used by subclasses for error reporting
    virtual clock_t getLockTime() { return m_lockTime; }

private:
    clock_t m_lockTime;
};


/* SimpleLockable can be locked by one client at a time.  It doesn't
   keep a record of which client; it assumes that a client will
   request a lock and only proceed if the lock succeeds (there's no
   authentication of further operations when the lock is in place).
 
   In the Rosegarden server, this is used for read-locks on individual
   elements, regardless of whether a transaction is also occurring (a
   transaction does not lock out read-only clients such as
   performers).
*/

template <int timeout = 5000> // five seconds
class SimpleLockable : public LockableBase<timeout>
{
public:
    SimpleLockable() : m_isLocked(false) { }
    virtual ~SimpleLockable();

    virtual bool l_lock(); // true if lock succeeded, false if already locked
    virtual void l_unlock();
    virtual bool l_isLocked();

protected:
    virtual bool hasSomeLock() { return m_isLocked; }

private:
    bool m_isLocked;
};


/** MultiLockable is a reference counter.  It can be locked many times
    at once, and only ceases to be locked when it's been unlocked as
    many times as locked.  This is overkill if you only want a pure
    reference counter, because of the timeout code.
 
    In the Rosegarden server, this was intended for use with the Part
    -- each iterator created would lock the Part and then unlock when
    released; the Part could not be deleted with extant locks.  The
    transaction code made this obsolete, because a Part could not be
    deleted when an iterator was in use for a transaction anyway.
    (Read-only clients need to check isOK() on their iterators
    constantly, but that's better than a r/w client being wholly
    unable to delete a Part because a r/o client is performing it.)
*/

template <int timeout = 30000> // half a minute
class MultiLockable : public LockableBase<timeout>
{
public:
    MultiLockable() : m_lockCount(0) { }
    virtual ~MultiLockable();

    virtual void lock();
    virtual void unlock();
    virtual bool isLocked();

protected:
    virtual bool hasSomeLock() { return m_lockCount > 0; }

private:
    short m_lockCount;
};


/** TransactionLockable can be locked by one client at once, and
    issues an id for each lock which other methods can authenticate
    against with an isGood() call.  This means an operation can be
    associated with a particular transaction, and mistakes (such as
    clients attempting to carry out an operation when another
    transaction is going on) reduced.
 
    In the Rosegarden Database, the Composition is a
    TransactionLockable and every mutating operation has to show a
    valid transaction id.  (database/impl/Transaction.[Ch] wrap this
    class; they also admit the possibility of null id for clients that
    are unable for threading reasons to respect locks, or for
    operations that are not part of a transaction.)
*/

template <int timeout = 60000> // one minute
class TransactionLockable : public LockableBase<timeout>
{
public:
    typedef long TransactionNumber;

    TransactionLockable() : m_transaction(0) { }
    virtual ~TransactionLockable();

    virtual bool lock(TransactionNumber);
    virtual void unlock(TransactionNumber);
    virtual bool isLocked();
    virtual bool isGood(TransactionNumber);
    virtual TransactionNumber newTransactionNumber();

protected:
    virtual bool hasSomeLock() { return m_transaction != 0; }

private:
    TransactionNumber m_transaction;
    static TransactionNumber m_nextTransaction;
};

#endif

#include "Lockable.cxx"

