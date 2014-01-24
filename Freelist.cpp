

#include "Freelist.h"
#include "WTPExceptions.h"
#include "WorkItem.h"
#include <iostream>

using namespace WTP;

void WTP::Freelist::addItem(WorkItem *wi)
{
	if (wi->getState() != WorkItem::IDLE)
		throw CallerError("WorkItem being added to freelist is not "
		                    "in IDLE state");
	wi->setState(WorkItem::FREELIST);
	lock();
	_wilist.push_back(wi);
	unlock();
}

WorkItem* WTP::Freelist::getItem()
{
	lock();
	if (_wilist.empty()) {
		unlock();
		return NULL;
	} else {
		WorkItem *wi = _wilist.back();
		_wilist.pop_back();
		unlock();
		if (wi->getState() != WorkItem::FREELIST)
			throw CallerError("WorkItem was modified while in "
			                    "freelist");
		wi->setState(WorkItem::IDLE);
		return wi;
	}
}

void WTP::Freelist::drain()
{
	lock();
	while (!_wilist.empty()) {
		WorkItem *wi = _wilist.back();
		_wilist.pop_back();
		delete wi;
	}
	unlock();
}


