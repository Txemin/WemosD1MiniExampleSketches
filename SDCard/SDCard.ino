#include <SdFat.h>
#include <SPI.h>

#define SD_SCK_PIN          14  // Wimos pin D5
#define SD_MISO_PIN         12  // Wimos pin D6
#define SD_MOSI_PIN         13  // Wimos pin D7
#define SD_CHIP_SELECT_PIN  15  // Wimos pin D8

const uint8_t SD_CHIP_SELECT = 15;

SdFat sd;
File myFile;
File myDir;

uint32_t cardSize;
uint32_t eraseSize;

void setup() {
  
  Serial.begin(115200);
  
  // Wait for USB Serial 
  while (!Serial)
    yield();

  initSDCard();
  
  Serial.print(F("SdFat version: "));
  Serial.println(SD_FAT_VERSION); 

  dumpSDInfo();
  Serial.println("");
  createDirectory();
  Serial.println("");
  writeToFile();
  Serial.println("");
  readFromFile();
  Serial.println("");
  readDirectory();
  Serial.println("");
  removeFolder();
  Serial.println("");
}

void loop() {
    
}

void readDirectory() {

  Serial.println("Reading directory test");

  if (myDir.open("/testFolder")) {

    while (myFile.openNext(&myDir, O_RDONLY)) {
      
      myFile.printFileSize(&Serial);
      Serial.write(' ');
      myFile.printModifyDateTime(&Serial);
      Serial.write(' ');
      myFile.printName(&Serial);
      
      if (myFile.isDir())
        Serial.write('/');
      
      Serial.println();
      myFile.close();
    }
  
  } else {
    
    Serial.println("open myDir failed");
  }  
}

void removeFolder() {
  
  Serial.println("Removing directory test");
  
  if (sd.rmdir("testFolder"))
    Serial.println("rmdir for 'testFolder' completed!");
  else
    Serial.println("rmdir for 'testFolder' failed");
}

void createDirectory() {

  Serial.println("Creating directory test");

  if (!sd.exists("testFolder")) {
    
    if (myDir.open("/")) {

      // Create a new folder.
      if (!sd.mkdir("testFolder"))
        Serial.println("Create 'testFolder' failed");
      
      Serial.println(F("Created 'testFolder'"));
      
    } else {
      
      Serial.println("open myDir failed");
    }  
    
  } else {

    Serial.println("Folder 'testFolder' already exists!");
  }
}

void writeToFile() {

  Serial.println("Writing to file test");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = sd.open("testFolder/test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
  
    Serial.print("Writing to 'testFolder/test.txt'...");
    myFile.println("testing 1, 2, 3.");
    // close the file:
    myFile.close();
    Serial.println("done.");
  
  } else {
    
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void readFromFile() {
  
  Serial.println("Reading from file test");

  // re-open the file for reading:
  myFile = sd.open("testFolder/test.txt");
  
  if (myFile) {
  
    Serial.println("'testFolder/test.txt' contains:");

    // read from the file until there's nothing else in it:
    while (myFile.available())
      Serial.write(myFile.read());
    
    // close the file:
    myFile.close();

  } else {
  
    // if the file didn't open, print an error:
    Serial.println("error opening 'testFolder/test.txt'");
  }
}

void initSDCard() {

  uint32_t t = millis();

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(SD_CHIP_SELECT, SD_SCK_MHZ(50))) {
    
    Serial.println("cardBegin failed");
    return;
  }

  t = millis() - t;

  cardSize = sd.card()->cardSize();
  
  if (cardSize == 0) {
  
    Serial.println("cardSize failed");
    return;
  }
  
  Serial.print(F("\ninit time: "));
  Serial.print(t);
  Serial.println(" ms");
}

void dumpSDInfo() {
    
  Serial.print(F("\nCard type: "));
  
  switch (sd.card()->type()) {
    
    case SD_CARD_TYPE_SD1:
      Serial.println(F("SD1"));
      break;
  
    case SD_CARD_TYPE_SD2:
      Serial.println(F("SD2"));
      break;
  
    case SD_CARD_TYPE_SDHC:
      if (cardSize < 70000000)
        Serial.println(F("SDHC"));
      else
        Serial.println(F("SDXC"));
      
      break;
  
    default:
      Serial.println(F("Unknown"));
  }
  
  if (!dumpCIDInformation())
    return;

  if (!dumpCSDInformation())
    return;

  uint32_t ocr;
  
  if (!sd.card()->readOCR(&ocr)) {
  
    Serial.println("\nreadOCR failed");
    return;
  }
  
  Serial.print(F("OCR: "));
  Serial.print("0X");
  Serial.println(ocr, HEX);
  
  if (!dumpPartitionInformation())
    return;
  
  if (!sd.fsBegin()) {
    Serial.println("\nFile System initialization failed.\n");
    return;
  }

  dumpVolumeInformation();
}

uint8_t dumpCIDInformation() {
  
  cid_t cid;
  
  if (!sd.card()->readCID(&cid)) {
    
    Serial.println("readCID failed");
    return false;
  }
  
  Serial.print(F("\nManufacturer ID: "));
  Serial.print("0X");
  Serial.println(int(cid.mid), HEX);
  Serial.println(F("OEM ID: ") + cid.oid[0] + cid.oid[1]);
  Serial.print(F("Product: "));
  
  for (uint8_t i = 0; i < 5; i++)
    Serial.print(cid.pnm[i]);
  
  Serial.println("");
  
  Serial.print(F("Version: "));
  Serial.print(int(cid.prv_n));
  Serial.print('.');
  Serial.println(int(cid.prv_m));
  
  Serial.print(F("Serial number: "));
  Serial.print("0X");
  Serial.println(cid.psn, HEX);
  
  Serial.print(F("Manufacturing date: "));
  Serial.print(int(cid.mdt_month));
  Serial.print('/');
  Serial.println((2000 + cid.mdt_year_low + 10 * cid.mdt_year_high));
  Serial.println("");
  return true;
}

uint8_t dumpCSDInformation() {
  
  csd_t csd;
  uint8_t eraseSingleBlock;
  
  if (!sd.card()->readCSD(&csd)) {
    
    Serial.println("readCSD failed");
    return false;
  }
  
  if (csd.v1.csd_ver == 0) {
    
    eraseSingleBlock = csd.v1.erase_blk_en;
    eraseSize = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
  
  } else if (csd.v2.csd_ver == 1) {
  
    eraseSingleBlock = csd.v2.erase_blk_en;
    eraseSize = (csd.v2.sector_size_high << 1) | csd.v2.sector_size_low;
  
  } else {
  
    Serial.println(F("csd version error"));
    return false;
  }
  
  eraseSize ++;
  Serial.print(F("cardSize: "));
  Serial.print(0.000512 * cardSize);
  Serial.println(F(" MB (MB = 1,000,000 bytes)"));

  Serial.print(F("flashEraseSize: "));
  Serial.print(int(eraseSize));
  Serial.println(F(" blocks"));
  
  Serial.print(F("eraseSingleBlock: "));
  
  if (eraseSingleBlock)
    Serial.println(F("true"));
  else
    Serial.println(F("false"));
  
  return true;
}

uint8_t dumpPartitionInformation() {
  
  mbr_t mbr;
  
  if (!sd.card()->readBlock(0, (uint8_t*)&mbr)) {
  
    Serial.println("read MBR failed");
    return false;
  }
  
  for (uint8_t ip = 1; ip < 5; ip++) {
  
    part_t *pt = &mbr.part[ip - 1];
    
    if ((pt->boot & 0X7F) != 0 || pt->firstSector > cardSize) {
    
      Serial.println(F("\nNo MBR. Assuming Super Floppy format."));
      return true;
    }
  }
  
  Serial.println(F("\nSD Partition Table"));
  Serial.println(F("part,boot,type,start,length"));
  
  for (uint8_t ip = 1; ip < 5; ip++) {
    
    part_t *pt = &mbr.part[ip - 1];
    Serial.print("0X");
    Serial.print(int(ip), HEX);
    Serial.print(',');
    Serial.print("0X");
    Serial.print(int(pt->boot), HEX);
    Serial.print(',');
    Serial.print("0X");
    Serial.print(int(pt->type), HEX);
    Serial.print(',');
    Serial.print(pt->firstSector);
    Serial.print(',');
    Serial.println(pt->totalSectors);
  }
  
  return true;
}

void dumpVolumeInformation() {
  
  Serial.print(F("\nVolume is FAT"));
  Serial.println(int(sd.vol()->fatType()));
  
  Serial.print(F("blocksPerCluster: "));
  Serial.println(int(sd.vol()->blocksPerCluster()));
  
  Serial.print(F("clusterCount: "));
  Serial.println(sd.vol()->clusterCount());
  
  Serial.print(F("freeClusters: "));
  uint32_t volFree = sd.vol()->freeClusterCount();
  Serial.println(volFree);
  
  float fs = 0.000512*volFree*sd.vol()->blocksPerCluster();
  Serial.print(F("freeSpace: "));
  Serial.print(fs);
  Serial.println(F(" MB (MB = 1,000,000 bytes)"));
  
  Serial.print(F("fatStartBlock: "));
  Serial.println(sd.vol()->fatStartBlock());
  
  Serial.print(F("fatCount: "));
  Serial.println(int(sd.vol()->fatCount()));
  
  Serial.print(F("blocksPerFat: "));
  Serial.println(sd.vol()->blocksPerFat());
  
  Serial.print(F("myDirDirStart: "));
  Serial.println(sd.vol()->rootDirStart());
  
  Serial.print(F("dataStartBlock: "));
  Serial.println(sd.vol()->dataStartBlock());
  
  if (sd.vol()->dataStartBlock() % eraseSize) {
  
    Serial.println(F("Data area is not aligned on flash erase boundaries!"));
    Serial.println(F("Download and use formatter from www.sdcard.org!"));
  }
}
