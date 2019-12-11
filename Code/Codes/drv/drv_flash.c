
#include "drv_flash.h"
#include "log.h"
//#include "fds.h"	//该版本sdk的fds库还不完善


#if (FLASH_DRV_VERSION == 1) && (!defined SOFTDEVICE_PRESENT)	//使用寄存器操作flash
#if 0
/**@brief flash页擦除
 * @param[in] page_address	擦除的页地址
 * @return
 */
static void flash_page_erase(uint32_t * page_address)
{
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);	//打开flash擦除使能
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)//等待操作完成
        sd_app_evt_wait();

    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;	//擦除整页
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
	
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	//打开flash擦除使能
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
}

/**@brief flash页擦除
 * @param[in] address	要写的地址
 * @param[in] value		要写的内容
 * @return
 */
static void flash_word_write(uint32_t * address, uint32_t value)
{
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);	//打开flash写使能
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();

    *address = value;	//写数据
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
	
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	//关闭flash写使能
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
}
#endif
#elif (FLASH_DRV_VERSION == 2) && (defined SOFTDEVICE_PRESENT)	//使用fstorage库
// 该协议栈版本fstorage并不完善
static uint32_t m_fs_start_addr;		///< 用户flash起始地址
static bool volatile m_fs_evt_write;
static bool volatile m_fs_evt_erase;

/**@brief flash操作事件处理函数 \n
 * 改函数由fstorage库里面的app_notify()触发回调；
 * 需要在协议栈初始化时注册softdevice_sys_evt_handler_set(sys_evt_dispatch)；
 * 将fs_sys_event_handler(sys_evt)添加到sys_evt_dispatch中。
 */
static void fs_evt_handle(uint8_t op_code, uint32_t result,
						uint32_t const * p_data, fs_length_t length_words)
{
	if (result != NRF_SUCCESS)
    {
        LOG_PRINT("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }
	else
	{
		if(FS_OP_STORE == op_code)
		{
			m_fs_evt_write = true;
//			LOG_PRINT("--> Event received: wrote %d bytes success.", length_words);
		}
		if(FS_OP_ERASE == op_code)
		{
			m_fs_evt_erase = true;
//			LOG_PRINT("--> Event received: erased %d bytes success.", length_words);
		}
	}
}

// 向 section fs_data中添加变量，fs_init会根据添加的自动分配flash
FS_SECTION_VARS_ADD(fs_config_t my_fs_config) = 
{
	.cb = fs_evt_handle,
    .num_pages = FLASH_DRV_USE_PAGES,	//1页1024字节
    .page_order = 1,		//分配优先级(越小优先级越高)
//    .p_start_addr = ,		//会在fs_init中自动分配
//    .p_end_addr = 
};

/**@brief fstorage 初始化
 */
static ret_code_t my_fs_init(void)
{
	ret_code_t rc;
	
	rc = fs_init();
	APP_ERROR_CHECK(rc);
	m_fs_start_addr = (uint32_t)my_fs_config.p_start_addr;
	
//	LOG_PRINT("FS_PAGE_END_ADDR: %08X\r\n", FS_PAGE_END_ADDR);
//	LOG_PRINT("flash strat addr: %08X\r\n", my_fs_config.p_start_addr);
//	LOG_PRINT("flash end addr: %08X\r\n", my_fs_config.p_end_addr);
	
    return rc;
}


#elif (FLASH_DRV_VERSION == 3) && (defined SOFTDEVICE_PRESENT)	//使用fds库
#if 0
// FDS事件完成标志
static bool volatile m_fds_evt_init;
static bool volatile m_fds_evt_write;
static bool volatile m_fds_evt_update;
static bool volatile m_fds_evt_del_record;
static bool volatile m_fds_evt_del_file;
static bool volatile m_fds_evt_gc;

/**@brief FDS事件处理函数
* @param[in]  *p_evt   	FDS事件指针
* @return none
* @note
*/
static void my_fds_evt_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:  //FDS初始化事件
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_init = true;
            }
            break;

        case FDS_EVT_WRITE: //FDS写记录事件
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Write Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
                m_fds_evt_write = true;
            }
        } break;
        
        case FDS_EVT_UPDATE:	//FDS更新事件，即写falsh操作
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_update = true;
            }
        } break;

        case FDS_EVT_DEL_RECORD:	//FDS删除记录事件
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Delete Record ID:\t0x%04x",  p_evt->del.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
                m_fds_evt_del_record = true;
            }
        } break;
        
        case FDS_EVT_DEL_FILE:		//FDS删除整个文件事件
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_del_file = true;
            }
        } break;
        
        case FDS_EVT_GC:			//FDS垃圾回收事件
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_gc = true;
            }
        } break;

        default:
            break;
    }
}

/**@brief FDS注册事件句柄和初始化程序 */
void my_fds_init(void)
{
    //注册FDS事件句柄，在调用fds_init()函数之前，一定要先注册
    ret_code_t rc = fds_register(my_fds_evt_handler);
    APP_ERROR_CHECK(rc);
    
    //FDS模块初始化
    m_fds_evt_init = false;
    rc = fds_init();
    APP_ERROR_CHECK(rc);
    //等待fds初始化完成
    while (!m_fds_evt_init)
    {
        sd_app_evt_wait();
    }
    
    fds_stat_t stat = {0};
    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);

    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
    if(stat.dirty_records>0)
    {
        m_fds_evt_gc = false;
        rc = fds_gc();
        APP_ERROR_CHECK(rc);
        while (!m_fds_evt_gc)
        {
            sd_app_evt_wait();
        }
    }
}

/**@brief FDS写记录函数
* @param[in] fid		文件 ID
* @param[in] key  		记录标号
* @param[in] *p_data   	要记录的数据指针
* @param[in] len   		要记录的数据长度
* @return 	 none
*/
void my_fds_write(uint32_t fid, uint32_t key, void const * p_data, uint32_t len)
{
    my_fds_find_and_delete(fid, key);
    
    fds_record_t const rec =
    {
        .file_id           = fid,
        .key               = key,
        .data.p_data       = p_data,
        .data.length_words = (len + 3) / sizeof(uint32_t)
    };
    m_fds_evt_write = false;
    ret_code_t rc = fds_record_write(NULL, &rec);
    if (rc != FDS_SUCCESS)
    {
        NRF_LOG_INFO("error: fds_record_write %d", rc);
    }
    else
    {
        while (!m_fds_evt_write)
        {
            sd_app_evt_wait();
        }
    }
}

/**@brief FDS读记录函数
* @param[in] fid		文件 ID
* @param[in] key  		记录标号
* @param[in] *p_data   	读取存储数据指针
* @param[in] len   		读取数据长度
* @return 	 none
*/
void my_fds_read(uint32_t fid, uint32_t key, void * p_data, uint32_t len)
{
    fds_record_desc_t desc = {0};
    fds_find_token_t  ftok = {0};
    if (fds_record_find(fid, key, &desc, &ftok) == FDS_SUCCESS)
    {
        fds_flash_record_t config = {0};    //存储读取的flash数据
        ret_code_t rc = fds_record_open(&desc, &config);//打开记录读取数据
        APP_ERROR_CHECK(rc);
        
        NRF_LOG_INFO("find record id = %d", desc.record_id);
        
        memcpy(p_data, config.p_data, len);
        
        rc = fds_record_close(&desc);   //关闭记录
        APP_ERROR_CHECK(rc);
    }
}

/**@brief FDS查找并删除记录标号的所有内容
* @param[in] fid		文件 ID
* @param[in] key  		记录标号
* @note 	 一个记录标号可以记录多个内容
*/
void my_fds_find_and_delete(uint32_t fid, uint32_t key)
{
    fds_record_desc_t desc = {0};
    fds_find_token_t  ftok = {0};
    while (fds_record_find(fid, key, &desc, &ftok) == FDS_SUCCESS)
    {
        m_fds_evt_del_record = false;
        fds_record_delete(&desc);
        while (!m_fds_evt_del_record)
        {
            sd_app_evt_wait();
        }
    }
    
    fds_stat_t stat = {0};
    ret_code_t rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);
    if(stat.dirty_records>0)
    {
        m_fds_evt_gc = false;
        rc = fds_gc();	//回收资源
        APP_ERROR_CHECK(rc);
        while (!m_fds_evt_gc)
        {
            sd_app_evt_wait();
        }
    }
}

/**@brief FDS删除整个文件
* @param[in] fid	文件 ID
*/
void my_fds_delete_file(uint32_t fid)
{
    ret_code_t rc;
    m_fds_evt_del_file = false;
    fds_file_delete(fid);
    while (!m_fds_evt_del_file)
    {
        sd_app_evt_wait();
    }
    
    m_fds_evt_gc = false;
    rc = fds_gc();
    APP_ERROR_CHECK(rc);
    while (!m_fds_evt_gc)
    {
        sd_app_evt_wait();
    }
}
#endif

#endif



/**@brief flash驱动初始化  \n
 * 		  主要是分配flash地址
 */
void drv_flash_init(void)
{
	my_fs_init();
}

/**@brief flash写操作(目前只考虑单扇区操作)
 * @param[in] addr		相对配置的起始地址位置
 * @param[in] data		要写入的数据的起始地址（需4字节对齐）
 * @param[in] len		要写入的数据的长度（需4字节倍数）
 * @return 	NRF_SUCCESS 成功
 */
ret_code_t drv_flash_write(uint32_t addr, uint8_t *data, uint16_t len)
{
	ret_code_t rc = NRF_SUCCESS;
	uint16_t i;
	uint8_t flash_buff[1024];
	
	if( (addr+len) > (FLASH_DRV_USE_PAGES*1024-1) || (len%4 != 0) )	//超出分配的flash区域,长度是否为4的倍数
		return NRF_ERROR_DATA_SIZE;
	if (!is_word_aligned((uint32_t const *)addr))	//检查地址是否4字节对齐
        return NRF_ERROR_INVALID_ADDR;
	if (!is_word_aligned((uint32_t const *)data))	//检查数据是否4字节对齐
		return NRF_ERROR_INVALID_DATA;
	
	
	uint8_t pagepos = addr / 1024;	//相对基址页地址
	uint16_t pageoff = addr % 1024;	//页内偏移地址
	uint16_t pageremain = 1024 - pageoff;	//页内剩余空间
	if(len <= pageremain) pageremain = len;
	
	while(1)
	{
		drv_flash_read(pagepos*1024, flash_buff, 1024);	//读出整个扇区的内容
		for(i=0; i<pageremain; i++)	// 验证要写的区域是否全0xFF,是则不用擦除
		{
			if(flash_buff[pageoff+i] != 0xFF)
				break;
		}
		if(i < pageremain)	//需要擦除
		{
			memcpy(&flash_buff[pageoff], data, pageremain);
			
			m_fs_evt_erase = false;
			rc = fs_erase(&my_fs_config, (uint32_t *)(m_fs_start_addr+pagepos*1024), 256);	//擦除整页
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_erase)
				sd_app_evt_wait();
			
			m_fs_evt_write = false;
			rc = fs_store(&my_fs_config, (uint32_t const *)(m_fs_start_addr+pagepos*1024), (uint32_t *)flash_buff, 256);
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_write)
				sd_app_evt_wait();
			
		}
		else	//不需要擦除直接写入相应位置
		{
			m_fs_evt_write = false;
			rc = fs_store(&my_fs_config, (uint32_t const *)(m_fs_start_addr + addr), (uint32_t *)data, pageremain/4);
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_write)
				sd_app_evt_wait();
		}
		
		if(len == pageremain)
			break;	//写入结束了
		else
		{
			pagepos++;				//页地址增1
			pageoff = 0;			//页偏移位置为0
			data += pageremain;		//数据指针偏移
			addr += pageremain;		//写地址偏移
			len -= pageremain;		//写长度递减
			if(len > 1024)
				pageremain = 1024;	//下一个扇区还是写不完
			else 
				pageremain = len;	//下一个扇区可以写完了
		}
			
	}
	
	return rc;
}

/**@brief flash写操作
 * @param[in] addr		相对配置的起始地址位置
 * @param[in] data		要写入的数据的起始地址（需4字节对齐）
 * @param[in] data		要写入的数据的长度（需4字节倍数）
 * @return 	NRF_SUCCESS 成功
 */
void drv_flash_read(uint32_t addr, uint8_t *data, uint16_t len)
{
	uint16_t i;
	uint32_t uladdr = m_fs_start_addr + addr;
	for(i=0; i<len; i++)
	{
		data[i] = *(__IO uint8_t *)(uladdr++); 
	}
}

#if 0
/**
 * @brief flash测试
 */
void flash_test(void)
{
	uint8_t test_data[4];
	
	drv_flash_read(4, test_data, 4);
	LOG_PRINT("flash_read: %02X %02X %02X %02X", test_data[0], test_data[1], test_data[2], test_data[3]);
}
#endif


