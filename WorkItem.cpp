
#include "WorkItem.h"
#include "Freelist.h"
#include "WorkerThreadPool.h"
#include "WTPExceptions.h"

#include <string>

using namespace WTP;

void WTP::WorkItem::returnToFreelist()
{
	if (_freelist) {
		_freelist->addItem(this);
	} else {
		throw InternalError("No freelist", __FILE__, __LINE__);
	}
}

void WTP::WorkItem::addSubItem(WorkItem *wi)
{
	if (getState() != RUNNING)
		throw CallerError("addSubItem() called from non-running work item");
	if (_subitem)
		throw CallerError("Attempt to add subitem from subitem");
	
	wi->_subitem = true;
	wi->_parent = this;
	incrRefcount();
	_threadPool->_addSubItem(wi, this);
}
