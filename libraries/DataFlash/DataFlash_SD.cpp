/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
 *
 */
#include <AP_HAL.h>
#include <MySD.h>
#include "DataFlash.h"
#include <stdlib.h>
#include <AP_Param.h>
#include <AP_Math.h>

extern const AP_HAL::HAL& hal;

#define PGM_UINT8(addr) pgm_read_byte((const prog_char *)addr)


/*
  try to take a semaphore safely from both in a timer and outside
 */

DataFlash_SD::DataFlash_SD() :
	_initialised(false)
{

}	    

// Public Methods //////////////////////////////////////////////////////////////
void DataFlash_SD::Init(const struct LogStructure *structure, uint8_t num_types)
{
    if( _initialised ) return ;
    DataFlash_Class::Init(structure, num_types);
    
    if( !SD.begin() )
    {
        hal.console->println_P(PSTR("DataFlash_SD not initialized. error\n"));
        return;
    }
    _initialised = true;
}

// This function return 1 if Card is inserted on SD slot
bool DataFlash_SD::CardInserted()
{
    return _initialised;
}

// erase handling
bool DataFlash_SD::NeedErase(void)
{
    // we could add a format marker at the start of a file?
    return false;
}

void DataFlash_SD::EraseAll()
{
    if( _currentFile )
	{
        _currentFile.close();
	}
    
    File dir = SD.open("/");
    dir.rewindDirectory();
    while (true) {
        File entry =  dir.openNextFile();
        if( !entry )
            break;
        if( !entry.isDirectory() )
            entry.remove();
        entry.close();
    }
    dir.close();

}

void DataFlash_SD::WriteBlock(const void *pBuffer, uint16_t size)
{
    if( !_initialised || !_currentFile ) return;
    
    if( !_currentFile.write(pBuffer, size))
        hal.console->println_P(PSTR("Write failed"));
    //_currentFile.flush();
}

void DataFlash_SD::EnableWrites(bool enable)
{
	if( enable )
	{
		if( !_currentFile )
			StartNewLog();
	}
	else 
		_currentFile.close();
	DataFlash_Class::EnableWrites(enable);
}


uint16_t  DataFlash_SD::find_last_log()
{
	return _get_file_count()-1;
}

void DataFlash_SD::get_log_boundaries(uint16_t log_num, uint16_t &start_page, uint16_t &end_page)
{
    if( !_initialised ) return;
    char buffer[13];
    getFileName(log_num, buffer, sizeof(buffer));
    if( SD.exists(buffer)) {
        File file = SD.open(buffer, O_READ);
        start_page = 0;
        end_page = file.size();
        file.close();
    }
}

uint16_t DataFlash_SD::get_num_logs(void)
{
	return _get_file_count() - 1;
}

void DataFlash_SD::LogReadProcess(uint16_t log_num,
                                  uint16_t start_page, uint16_t end_page,
                                  void (*printMode)(AP_HAL::BetterStream *port, uint8_t mode),
                                  AP_HAL::BetterStream *port)
{
    uint8_t log_step = 0;
    bool first_entry = true;
    char buffer[13];
    getFileName(log_num, buffer, sizeof(buffer));
    File file = SD.open(buffer, O_READ);
    while ( true ) {
        uint8_t data;
        data = file.read();
        if( data == -1 ) break;
        switch (log_step) {
            case 0:
                if( data == HEAD_BYTE1)
                    log_step++;
                break;
            case 1:
                if(data == HEAD_BYTE2)
                    log_step++;
                else
                    log_step = 0;
            case 2:
                log_step = 0;
                if( first_entry && data != LOG_FORMAT_MSG)
                    _print_log_formats(port);
                first_entry = false;
                _print_log_entry(data, printMode, port);
                break;
        }
    }
    file.close();
}

void DataFlash_SD::DumpPageInfo(AP_HAL::BetterStream *port)
{
    port->printf_P(PSTR("DataFlash: num_logs=%u\n"),
                   (unsigned)get_num_logs());
}

void DataFlash_SD::ShowDeviceInfo(AP_HAL::BetterStream *port)
{
    //port->printf_P(PSTR("DataFlash logs stored in %s\n"),
    //               _log_directory);
}

void DataFlash_SD::ListAvailableLogs(AP_HAL::BetterStream *port)
{
    uint16_t num_logs = get_num_logs();
    if (num_logs == 0) {
        port->printf_P(PSTR("\nNo logs\n\n"));
        return;
    }
    port->printf_P(PSTR("\n%u logs\n"), (unsigned)num_logs);
    
    File dir = SD.open("/");
    dir.rewindDirectory();
    num_logs = 0;
    while (true) {
        File entry =  dir.openNextFile();
        if( !entry )
            break;
        if( !entry.isDirectory() )
            port->printf_P(PSTR("%s\n"), entry.name());
        entry.close();
    }
    dir.close();
}

uint16_t DataFlash_SD::start_new_log(void)
{
    if( !_initialised) return 0;
    
    if( _currentFile )
        _currentFile.close();

    uint8_t log_num = _get_file_count();
    char buffer[13];
    getFileName(log_num, buffer, sizeof(buffer));
    
    _currentFile = SD.open(buffer, O_CREAT|O_WRITE|O_APPEND);
    if( !_currentFile )
        hal.console->print_P(PSTR("File open error\n"));
    
	return log_num;
}

void DataFlash_SD::ReadBlock(void *pkt, uint16_t size)
{
    if( !_initialised || !_currentFile ) return;
    
    if( !_currentFile.read(pkt, size) )
        hal.console->println_P(PSTR("Read failed"));
}

void DataFlash_SD::get_log_info(uint16_t log_num, uint32_t &size, uint32_t &time_utc)
{
    char buffer[13];
    getFileName(log_num, buffer, sizeof(buffer));
    File file = SD.open(buffer, O_READ);
    size = file.size();
    file.close();
    
}

int16_t DataFlash_SD::get_log_data(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t*buf)
{
    char buffer[13];
    getFileName(log_num, buffer, sizeof(buffer));
    File file = SD.open(buffer, O_READ);
    file.seek(offset);
    int size = file.read(buf, len);
    file.close();
    return size;
}



/*
    private members
 */

void DataFlash_SD::_print_log_formats(AP_HAL::BetterStream *port)
{
    for (uint8_t i=0; i<_num_types; i++) {
        const struct LogStructure *s = &_structures[i];
        port->printf_P(PSTR("FMT, %u, %u, %S, %S, %S\n"),
                       (unsigned)PGM_UINT8(&s->msg_type),
                       (unsigned)PGM_UINT8(&s->msg_len),
                       s->name, s->format, s->labels);
    }
}

uint16_t DataFlash_SD::_get_file_count()
{
    if( !_initialised) return 0;

    File dir = SD.open("/");
    dir.rewindDirectory();
    uint16_t fileCount = 1;
    
    while(true)
    {
        File entry =  dir.openNextFile();
        if( !entry )
            break;
        if( !entry.isDirectory() ) fileCount++;
        entry.close();
    }
    dir.close();
        
    return fileCount;
}

void DataFlash_SD::getFileName(uint16_t fileNum, char *buffer, int16_t size)
{
    //char buffer[13];
    memset(buffer, size, 0);
    hal.util->snprintf(buffer, 12, "%08u.bin", fileNum);
    buffer[size-1]='\0';
    //return buffer;
}



