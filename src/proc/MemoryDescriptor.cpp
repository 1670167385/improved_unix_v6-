#include "MemoryDescriptor.h"
#include "Kernel.h"
#include "PageManager.h"
#include "Machine.h"
#include "PageDirectory.h"
#include "Video.h"

void MemoryDescriptor::Initialize()
{
	/* m_UserPageTableArray需要把AllocMemory()返回的物理内存地址 + 0xC0000000 */
	this->m_UserPageTableArray = NULL;
}

void MemoryDescriptor::Release()
{
	KernelPageManager& kernelPageManager = Kernel::Instance().GetKernelPageManager();
	if ( this->m_UserPageTableArray )
	{
		kernelPageManager.FreeMemory(sizeof(PageTable) * USER_SPACE_PAGE_TABLE_CNT, (unsigned long)this->m_UserPageTableArray - Machine::KERNEL_SPACE_START_ADDRESS);
		this->m_UserPageTableArray = NULL;
	}
}

unsigned int MemoryDescriptor::MapEntry(unsigned long virtualAddress, unsigned int size, unsigned long phyPageIdx, bool isReadWrite)
{	
	return phyPageIdx;
}

void MemoryDescriptor::MapTextEntrys(unsigned long textStartAddress, unsigned long textSize, unsigned long textPageIdx)
{
	this->MapEntry(textStartAddress, textSize, textPageIdx, false);
}
void MemoryDescriptor::MapDataEntrys(unsigned long dataStartAddress, unsigned long dataSize, unsigned long dataPageIdx)
{
	this->MapEntry(dataStartAddress, dataSize, dataPageIdx, true);
}

void MemoryDescriptor::MapStackEntrys(unsigned long stackSize, unsigned long stackPageIdx)
{
	unsigned long stackStartAddress = (USER_SPACE_START_ADDRESS + USER_SPACE_SIZE - stackSize) & 0xFFFFF000;
	this->MapEntry(stackStartAddress, stackSize, stackPageIdx, true);
}

PageTable* MemoryDescriptor::GetUserPageTableArray()
{
	return NULL;
}
unsigned long MemoryDescriptor::GetTextStartAddress()
{
	return this->m_TextStartAddress;
}
unsigned long MemoryDescriptor::GetTextSize()
{
	return this->m_TextSize;
}
unsigned long MemoryDescriptor::GetDataStartAddress()
{
	return this->m_DataStartAddress;
}
unsigned long MemoryDescriptor::GetDataSize()
{
	return this->m_DataSize;
}
unsigned long MemoryDescriptor::GetStackSize()
{
	return this->m_StackSize;
}

bool MemoryDescriptor::EstablishUserPageTable( unsigned long textVirtualAddress, unsigned long textSize, unsigned long dataVirtualAddress, unsigned long dataSize, unsigned long stackSize )
{
	User& u = Kernel::Instance().GetUser();

	/* 如果超出允许的用户程序最大8M的地址空间限制 */
	if ( textSize + dataSize + stackSize  + PageManager::PAGE_SIZE > USER_SPACE_SIZE - textVirtualAddress)
	{
		u.u_error = User::ENOMEM;
		Diagnose::Write("u.u_error = %d\n",u.u_error);
		return false;
	}

	m_TextSize=textSize;
	m_DataSize=dataSize;
	m_StackSize=stackSize;

	/* 将相对地址映照表根据正文段和数据段在内存中的起始地址pText->x_caddr、p_addr，建立用户态内存区的页表映射 */
	this->MapToPageTable();
	return true;
}

void MemoryDescriptor::ClearUserPageTable()
{
	;
}

void MemoryDescriptor::MapToPageTable()
{
	User& u = Kernel::Instance().GetUser();
	PageTable* pUserPageTable = Machine::Instance().GetUserPageTableArray(); 
	unsigned int textAddress = 0;
	if (u.u_procp->p_textp != NULL) { 
        textAddress = u.u_procp->p_textp->x_caddr; 
    } 
	unsigned int tstart_index = 0, dstart_index = 1;//代码段和数据段起始偏移噩 
    unsigned int text_len = (m_TextSize + (PageManager::PAGE_SIZE - 1)) / PageManager::PAGE_SIZE; 
    unsigned int data_len = (m_DataSize + (PageManager::PAGE_SIZE - 1)) / PageManager::PAGE_SIZE; 
    unsigned int stack_len = (m_StackSize+ (PageManager::PAGE_SIZE - 1)) / PageManager::PAGE_SIZE; 
	unsigned int dataidx = 0; //data段的页框号计数

	for (unsigned int i = 0; i < Machine::USER_PAGE_TABLE_CNT; i++)
	{
		for ( unsigned int j = 0; j < PageTable::ENTRY_CNT_PER_PAGETABLE; j++ )
		{
			pUserPageTable[i].m_Entrys[j].m_Present = 0;   //先清0

			if ( 1 == i )
			{
				/* 只读属性表示正文段对应的页，以pText->x_caddr为内存起始地址 */
				if ( 1 <= j && j <=text_len )
				{
					pUserPageTable[i].m_Entrys[j].m_Present = 1;
					pUserPageTable[i].m_Entrys[j].m_ReadWriter = 0;
					pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = j-1 + tstart_index + (textAddress >> 12);
				}
				/* 读写属性表示数据段对应的页，以p_addr为内存起始地址 */
				else if ( j > text_len && j <= text_len + data_len )
				{
					pUserPageTable[i].m_Entrys[j].m_Present = 1;
					pUserPageTable[i].m_Entrys[j].m_ReadWriter = 1;
					pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = dataidx + dstart_index + (u.u_procp->p_addr >> 12);
					dataidx++;
				}
				/* 堆栈段 1024 - stack_len */
				else if(j >= PageTable::ENTRY_CNT_PER_PAGETABLE-stack_len)
				{
					pUserPageTable[i].m_Entrys[j].m_Present = 1;
					pUserPageTable[i].m_Entrys[j].m_ReadWriter = 1;
					pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = dataidx + dstart_index + (u.u_procp->p_addr >> 12);
					dataidx++;
				}
			}
		}
	}

	pUserPageTable[0].m_Entrys[0].m_Present = 1;
	pUserPageTable[0].m_Entrys[0].m_ReadWriter = 1;
	pUserPageTable[0].m_Entrys[0].m_PageBaseAddress = 0;

	FlushPageDirectory();
}
