#include "biqu.h"
#include "SdBaseFile.h"
#include "Marlin.h"
#include "cardreader.h"


#if ENABLED(POWER_FAILED_SENSOR)

bool power_failed_save=false;
bool hasPrintingSys=false;
void gcode_G28(const bool always_home_all) ;

/**
 * Power failued Kill all heater.
 * 
 */
void killHeater(const char* lcd_msg) 
{
  #if HAS_TEMP_BED
	WRITE_HEATER_BED(LOW);
  #endif
  
#if HAS_TEMP_HOTEND
	WRITE_HEATER_0(LOW);
  #if HOTENDS > 1
	WRITE_HEATER_1(LOW);
	#if HOTENDS > 2
	  WRITE_HEATER_2(LOW);
	  #if HOTENDS > 3
	    WRITE_HEATER_3(LOW);
		#if HOTENDS > 4
	      WRITE_HEATER_4(LOW);
		#endif // HOTENDS > 4
	  #endif // HOTENDS > 3
	#endif // HOTENDS > 2
  #endif // HOTENDS > 1
#endif

	card.powerFailedSave();
	disable_e_steppers();

#if ENABLED(ULTRA_LCD)
	lcd_setalertstatusPGM(lcd_msg);
#endif
	thermalManager.disable_all_heaters(); //turn off heaters
	clear_command_queue();
	
	//stepper.cleaning_buffer_counter = 5000;
	DISABLE_STEPPER_DRIVER_INTERRUPT();
	while (planner.blocks_queued()) planner.discard_current_block();
	stepper.current_block = NULL;
	ENABLE_STEPPER_DRIVER_INTERRUPT();
	#if ENABLED(ULTRA_LCD)
	  planner.clear_block_buffer_runtime();
	#endif
   //enqueue_and_echo_command("M995\n");
}

char printFile[LONG_FILENAME_LENGTH];
void CardReader::powerFailedSave(void) 
{
  if (!cardOK || !sdprinting) return;
  
	char flag=0;
	char buf[MAX_CMD_SIZE];
	stopSDPrint();

	if (file.open(&root, "printing.sys", O_CREAT | O_APPEND | O_WRITE | O_TRUNC)) 
	{
		//x y z e f offset nozzle bed fan path
		file.write(destination,sizeof(destination));
		flag |= file.writeError;
		file.write(&feedrate_mm_s,sizeof(feedrate_mm_s));
		flag |= file.writeError<<1;

		
		
		file.write(&sdpos,sizeof(sdpos));
		flag |= file.writeError<<2;
		file.write(thermalManager.target_temperature,sizeof(thermalManager.target_temperature));
		flag |= file.writeError<<3;
		file.write(&thermalManager.target_temperature_bed,sizeof(thermalManager.target_temperature_bed));
		flag |= file.writeError<<4;
		file.write(fanSpeeds,sizeof(fanSpeeds));
		flag |= file.writeError<<5;
		file.write(printFile,sizeof(printFile));
		flag |= file.writeError<<6;
		
		file.sync();
		file.close();
		if (flag) 
		{
			SERIAL_PROTOCOLLNPGM("save failed");
		}
		else
		{
            hasPrintingSys=true;
			SERIAL_PROTOCOLLNPGM("save ok");
		}
	}	
  else
  {
    SERIAL_PROTOCOLLNPGM("save failed");
  }
}

void sync_plan_position_e();
void CardReader::powerFailedLoad(void) 
{
  if (!cardOK) return;
 
	float tempPosition[XYZE];
	uint32_t offset;
	char buf[MAX_CMD_SIZE];
 
	if (file.open( &root, "printing.sys", O_READ)) 
	{
	//x y z e f offset nozzle bed fan path
		file.read(tempPosition,sizeof(tempPosition));
		file.read(&feedrate_mm_s,sizeof(feedrate_mm_s));
		file.read(&offset,sizeof(offset));
		file.read(thermalManager.target_temperature,sizeof(thermalManager.target_temperature));
		file.read(&thermalManager.target_temperature_bed,sizeof(thermalManager.target_temperature_bed));
		file.read(fanSpeeds,sizeof(fanSpeeds));
		file.read(printFile,sizeof(printFile));
		file.close();
		if (file.writeError) 
		{
			SERIAL_PROTOCOLLNPGM("Load failed");
		}
		else
		{	
			openFile(printFile, true);
			sdpos = offset; 
			file.seekSet(offset);
			startFileprint();
#if ENABLED(ULTRA_LCD)
      void lcd_return_to_status(void);
			lcd_setstatus(printFile);
      lcd_return_to_status();
#endif
			while(thermalManager.current_temperature[0]<thermalManager.target_temperature[0]-1)
			{
				idle();
			}
      if(sdprinting)
      {
    		gcode_G28(false);
			//enqueue_and_echo_command("G28\n");
			
			//sprintf(buf,"G1 X%d.%03d Y%d.%03d Z%d.%03d F%d\n",int(tempPosition[0]), int((tempPosition[0] - int(tempPosition[0]))*1000),int(tempPosition[1]), int((tempPosition[1] - int(tempPosition[1]))*1000),int(tempPosition[2]), int((tempPosition[2] - int(tempPosition[2]))*1000),int(feedrate_mm_s*60));
			//enqueue_and_echo_command(buf,true);
			 COPY(destination,tempPosition);

       stepper.synchronize();
       current_position[E_AXIS]=destination[E_AXIS];
       sync_plan_position_e();
      
			 prepare_move_to_destination();
      }
			SERIAL_PROTOCOLLNPGM("Load ok");
		}
	}	
}


/*


sprintf(cmdOutage[0],"M109 S%d\n",infoOutage.nozzle);
			sprintf(cmdOutage[1],"G28\n");
			sprintf(cmdOutage[2],"M190 S%d\n",infoOutage.bed);
			sprintf(cmdOutage[3],"G28\n");
			sprintf(cmdOutage[4],"G21\n");
			sprintf(cmdOutage[5],"G90\n");
			sprintf(cmdOutage[6],"M106 S%d\n",infoOutage.fan);
			sprintf(cmdOutage[7],"G92 E0\n");
			sprintf(cmdOutage[8],"G1 E5.5\n");
			sprintf(cmdOutage[9],"G92 E%f\n",infoOutage.e);
			sprintf(cmdOutage[10],"G1 X%f Y%f Z%f F%f\n",infoOutage.x,infoOutage.y,infoOutage.z,infoOutage.f);



			*/

#endif

