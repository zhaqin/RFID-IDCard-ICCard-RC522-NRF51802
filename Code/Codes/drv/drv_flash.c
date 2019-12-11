
#include "drv_flash.h"
#include "log.h"
//#include "fds.h"	//�ð汾sdk��fds�⻹������


#if (FLASH_DRV_VERSION == 1) && (!defined SOFTDEVICE_PRESENT)	//ʹ�üĴ�������flash
#if 0
/**@brief flashҳ����
 * @param[in] page_address	������ҳ��ַ
 * @return
 */
static void flash_page_erase(uint32_t * page_address)
{
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);	//��flash����ʹ��
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)//�ȴ��������
        sd_app_evt_wait();

    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;	//������ҳ
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
	
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	//��flash����ʹ��
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
}

/**@brief flashҳ����
 * @param[in] address	Ҫд�ĵ�ַ
 * @param[in] value		Ҫд������
 * @return
 */
static void flash_word_write(uint32_t * address, uint32_t value)
{
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);	//��flashдʹ��
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();

    *address = value;	//д����
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
	
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	//�ر�flashдʹ��
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
		sd_app_evt_wait();
}
#endif
#elif (FLASH_DRV_VERSION == 2) && (defined SOFTDEVICE_PRESENT)	//ʹ��fstorage��
// ��Э��ջ�汾fstorage��������
static uint32_t m_fs_start_addr;		///< �û�flash��ʼ��ַ
static bool volatile m_fs_evt_write;
static bool volatile m_fs_evt_erase;

/**@brief flash�����¼������� \n
 * �ĺ�����fstorage�������app_notify()�����ص���
 * ��Ҫ��Э��ջ��ʼ��ʱע��softdevice_sys_evt_handler_set(sys_evt_dispatch)��
 * ��fs_sys_event_handler(sys_evt)��ӵ�sys_evt_dispatch�С�
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

// �� section fs_data����ӱ�����fs_init�������ӵ��Զ�����flash
FS_SECTION_VARS_ADD(fs_config_t my_fs_config) = 
{
	.cb = fs_evt_handle,
    .num_pages = FLASH_DRV_USE_PAGES,	//1ҳ1024�ֽ�
    .page_order = 1,		//�������ȼ�(ԽС���ȼ�Խ��)
//    .p_start_addr = ,		//����fs_init���Զ�����
//    .p_end_addr = 
};

/**@brief fstorage ��ʼ��
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


#elif (FLASH_DRV_VERSION == 3) && (defined SOFTDEVICE_PRESENT)	//ʹ��fds��
#if 0
// FDS�¼���ɱ�־
static bool volatile m_fds_evt_init;
static bool volatile m_fds_evt_write;
static bool volatile m_fds_evt_update;
static bool volatile m_fds_evt_del_record;
static bool volatile m_fds_evt_del_file;
static bool volatile m_fds_evt_gc;

/**@brief FDS�¼�������
* @param[in]  *p_evt   	FDS�¼�ָ��
* @return none
* @note
*/
static void my_fds_evt_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:  //FDS��ʼ���¼�
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_init = true;
            }
            break;

        case FDS_EVT_WRITE: //FDSд��¼�¼�
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Write Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
                m_fds_evt_write = true;
            }
        } break;
        
        case FDS_EVT_UPDATE:	//FDS�����¼�����дfalsh����
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_update = true;
            }
        } break;

        case FDS_EVT_DEL_RECORD:	//FDSɾ����¼�¼�
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Delete Record ID:\t0x%04x",  p_evt->del.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
                m_fds_evt_del_record = true;
            }
        } break;
        
        case FDS_EVT_DEL_FILE:		//FDSɾ�������ļ��¼�
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_evt_del_file = true;
            }
        } break;
        
        case FDS_EVT_GC:			//FDS���������¼�
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

/**@brief FDSע���¼�����ͳ�ʼ������ */
void my_fds_init(void)
{
    //ע��FDS�¼�������ڵ���fds_init()����֮ǰ��һ��Ҫ��ע��
    ret_code_t rc = fds_register(my_fds_evt_handler);
    APP_ERROR_CHECK(rc);
    
    //FDSģ���ʼ��
    m_fds_evt_init = false;
    rc = fds_init();
    APP_ERROR_CHECK(rc);
    //�ȴ�fds��ʼ�����
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

/**@brief FDSд��¼����
* @param[in] fid		�ļ� ID
* @param[in] key  		��¼���
* @param[in] *p_data   	Ҫ��¼������ָ��
* @param[in] len   		Ҫ��¼�����ݳ���
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

/**@brief FDS����¼����
* @param[in] fid		�ļ� ID
* @param[in] key  		��¼���
* @param[in] *p_data   	��ȡ�洢����ָ��
* @param[in] len   		��ȡ���ݳ���
* @return 	 none
*/
void my_fds_read(uint32_t fid, uint32_t key, void * p_data, uint32_t len)
{
    fds_record_desc_t desc = {0};
    fds_find_token_t  ftok = {0};
    if (fds_record_find(fid, key, &desc, &ftok) == FDS_SUCCESS)
    {
        fds_flash_record_t config = {0};    //�洢��ȡ��flash����
        ret_code_t rc = fds_record_open(&desc, &config);//�򿪼�¼��ȡ����
        APP_ERROR_CHECK(rc);
        
        NRF_LOG_INFO("find record id = %d", desc.record_id);
        
        memcpy(p_data, config.p_data, len);
        
        rc = fds_record_close(&desc);   //�رռ�¼
        APP_ERROR_CHECK(rc);
    }
}

/**@brief FDS���Ҳ�ɾ����¼��ŵ���������
* @param[in] fid		�ļ� ID
* @param[in] key  		��¼���
* @note 	 һ����¼��ſ��Լ�¼�������
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
        rc = fds_gc();	//������Դ
        APP_ERROR_CHECK(rc);
        while (!m_fds_evt_gc)
        {
            sd_app_evt_wait();
        }
    }
}

/**@brief FDSɾ�������ļ�
* @param[in] fid	�ļ� ID
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



/**@brief flash������ʼ��  \n
 * 		  ��Ҫ�Ƿ���flash��ַ
 */
void drv_flash_init(void)
{
	my_fs_init();
}

/**@brief flashд����(Ŀǰֻ���ǵ���������)
 * @param[in] addr		������õ���ʼ��ַλ��
 * @param[in] data		Ҫд������ݵ���ʼ��ַ����4�ֽڶ��룩
 * @param[in] len		Ҫд������ݵĳ��ȣ���4�ֽڱ�����
 * @return 	NRF_SUCCESS �ɹ�
 */
ret_code_t drv_flash_write(uint32_t addr, uint8_t *data, uint16_t len)
{
	ret_code_t rc = NRF_SUCCESS;
	uint16_t i;
	uint8_t flash_buff[1024];
	
	if( (addr+len) > (FLASH_DRV_USE_PAGES*1024-1) || (len%4 != 0) )	//���������flash����,�����Ƿ�Ϊ4�ı���
		return NRF_ERROR_DATA_SIZE;
	if (!is_word_aligned((uint32_t const *)addr))	//����ַ�Ƿ�4�ֽڶ���
        return NRF_ERROR_INVALID_ADDR;
	if (!is_word_aligned((uint32_t const *)data))	//��������Ƿ�4�ֽڶ���
		return NRF_ERROR_INVALID_DATA;
	
	
	uint8_t pagepos = addr / 1024;	//��Ի�ַҳ��ַ
	uint16_t pageoff = addr % 1024;	//ҳ��ƫ�Ƶ�ַ
	uint16_t pageremain = 1024 - pageoff;	//ҳ��ʣ��ռ�
	if(len <= pageremain) pageremain = len;
	
	while(1)
	{
		drv_flash_read(pagepos*1024, flash_buff, 1024);	//������������������
		for(i=0; i<pageremain; i++)	// ��֤Ҫд�������Ƿ�ȫ0xFF,�����ò���
		{
			if(flash_buff[pageoff+i] != 0xFF)
				break;
		}
		if(i < pageremain)	//��Ҫ����
		{
			memcpy(&flash_buff[pageoff], data, pageremain);
			
			m_fs_evt_erase = false;
			rc = fs_erase(&my_fs_config, (uint32_t *)(m_fs_start_addr+pagepos*1024), 256);	//������ҳ
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_erase)
				sd_app_evt_wait();
			
			m_fs_evt_write = false;
			rc = fs_store(&my_fs_config, (uint32_t const *)(m_fs_start_addr+pagepos*1024), (uint32_t *)flash_buff, 256);
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_write)
				sd_app_evt_wait();
			
		}
		else	//����Ҫ����ֱ��д����Ӧλ��
		{
			m_fs_evt_write = false;
			rc = fs_store(&my_fs_config, (uint32_t const *)(m_fs_start_addr + addr), (uint32_t *)data, pageremain/4);
			APP_ERROR_CHECK(rc);
			while(!m_fs_evt_write)
				sd_app_evt_wait();
		}
		
		if(len == pageremain)
			break;	//д�������
		else
		{
			pagepos++;				//ҳ��ַ��1
			pageoff = 0;			//ҳƫ��λ��Ϊ0
			data += pageremain;		//����ָ��ƫ��
			addr += pageremain;		//д��ַƫ��
			len -= pageremain;		//д���ȵݼ�
			if(len > 1024)
				pageremain = 1024;	//��һ����������д����
			else 
				pageremain = len;	//��һ����������д����
		}
			
	}
	
	return rc;
}

/**@brief flashд����
 * @param[in] addr		������õ���ʼ��ַλ��
 * @param[in] data		Ҫд������ݵ���ʼ��ַ����4�ֽڶ��룩
 * @param[in] data		Ҫд������ݵĳ��ȣ���4�ֽڱ�����
 * @return 	NRF_SUCCESS �ɹ�
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
 * @brief flash����
 */
void flash_test(void)
{
	uint8_t test_data[4];
	
	drv_flash_read(4, test_data, 4);
	LOG_PRINT("flash_read: %02X %02X %02X %02X", test_data[0], test_data[1], test_data[2], test_data[3]);
}
#endif


