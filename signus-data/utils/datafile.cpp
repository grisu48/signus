//
// Modul datovych souboru
//


#include <string.h>
#include "datafile.h"

#define fopensafe fopen


/////////////////////////////// STD funkce i/o /////////////////////////////


extern FILE *fopensafe(const char *name, const char *mode);
// fce obsazena v global.cpp, neni tu inc global.h, protoze to se moc
// casto meni, tenhle modul chce mit taky chvilku klidu...



unsigned StdDataWrite(FILE *f, void *ptr, size_t size)
{
	fwrite(&size, sizeof(size), 1, f);
	fwrite(ptr, size, 1, f);
	return (sizeof(size) + size);
}



void *StdDataRead(FILE *f)
{
	void *ptr;
	size_t size;
	
	fread(&size, sizeof(size), 1, f);
	ptr = malloc(size);
	if (ptr != NULL) 
		fread(ptr, size, 1, f);

	return ptr;
}






///////////////////////////// trida TDataFile //////////////////////////////




TDataFile::TDataFile(char *name, int flags, char *aprefix, char areplac,
			               TDataWriteFce wfce, TDataReadFce rfce)
{
	writefce = wfce;
	readfce = rfce;
	changed = 0;
	readprefix = (aprefix == NULL) ? "" : aprefix;
	readreplacer = areplac;

	if (flags & dfCreate) {     // vytvor novy datak
		resf = fopensafe(name, "wb");
		count = 0;
		index = NULL;		
	}
	
	if (flags & dfOpenRead) {   // otevre pro cteni a nacte tabulku
		unsigned tableofs;
		resf = fopensafe(name, "rb");
		fread(&tableofs, sizeof(tableofs), 1, resf);
		fseek(resf, tableofs, SEEK_SET);
		fread(&count, sizeof(count), 1, resf);
		index = (TDataIndex *) malloc(count * sizeof(TDataIndex));
		fread(index, sizeof(TDataIndex) * count, 1, resf);				
	}
	
	if (flags & dfOpenWrite) {  // otevre por zapis
		unsigned tableofs;
		resf = fopensafe(name, "rb+");
		fread(&tableofs, sizeof(tableofs), 1, resf);
		fseek(resf, tableofs, SEEK_SET);
		fread(&count, sizeof(count), 1, resf);
		index = (TDataIndex *) malloc(count * sizeof(TDataIndex));
		fread(index, sizeof(TDataIndex) * count, 1, resf);				
		sortindex(0);
	}
}



			
int TDataFile::put(char *name, void *ptr, size_t size)
{
	int indx = -1;

	// test existence:
	for (int i = 0; i < count; i++) 
		if (strcmp(name, index[i].name) == 0) {
			indx = i; break;
		}
		
	// nahrazeni stare polozky (musi mit stejnou velikost):
	if ((indx != -1)) {
		int oldsize = index[indx].size;
		changed = 1;
		fseek(resf, index[indx].offset, SEEK_SET);
		index[indx].size = writefce(resf, ptr, size);
		if (oldsize != index[indx].size) {return 0;}
	}	

  // pridani nove polozky:
  else {
		changed = 1;
		count++;
		index = (TDataIndex *) realloc(index, sizeof(TDataIndex) * count);
		strcpy(index[count-1].name, name);
		if (count == 1) 
			index[0].offset = 4;
		else
			index[count-1].offset = index[count-2].offset + index[count-2].size;
		fseek(resf, index[count-1].offset, SEEK_SET);
		index[count-1].size = writefce(resf, ptr, size);
	}
	return 1;
}



int TDataFile::lookfor(char *name, int lo, int hi)
{
	int pos = (hi + lo) / 2;
	int res = strcmp(name, index[pos].name);
	if (res == 0) return pos;
	else {
		if (pos == lo) 
			if (strcmp(name, index[hi].name) != 0) return -1;
		if (pos == hi) 
		  if (strcmp(name, index[lo].name) != 0) return -1;			
		if (res < 0) return lookfor(name, lo, pos-1);
		else return lookfor(name, pos+1, hi);
	}
}



void *TDataFile::get(char *name)
{
	void *ptr = NULL;
	char nm[9];
	
	strcpy(nm, readprefix);
	strcat(nm, name);
	if (nm[0] == '?') nm[0] = readreplacer;
	
	int i = lookfor(nm, 0, count-1);
	if (i != -1) {
		fseek(resf, index[i].offset, SEEK_SET);
		return readfce(resf);
	}
	return ptr;
}



int compare_by_name(const void *o1, const void *o2)
{
	TDataIndex *i1 = (TDataIndex*)o1;
	TDataIndex *i2 = (TDataIndex*)o2;
	return strcmp(i1->name, i2->name);
}

int compare_by_offset(const void *o1, const void *o2)
{
	TDataIndex *i1 = (TDataIndex*)o1;
	TDataIndex *i2 = (TDataIndex*)o2;
	return (i1->offset) - (i2->offset);
}



void TDataFile::sortindex(int bywhat)
{
	if (bywhat == 1)  // compare by name
		qsort(index, count, sizeof(TDataIndex), compare_by_name);
	else 
		qsort(index, count, sizeof(TDataIndex), compare_by_offset);	
}



TDataFile::~TDataFile()
{
	if (changed) {      // provedeny zmeny v souboru, musi se zapsat tabulka
		unsigned tableofs = 4 + index[count-1].offset + index[count-1].size;
	
		fseek(resf, 0, SEEK_SET);
		fwrite(&tableofs, sizeof(tableofs), 1, resf);
		fseek(resf, tableofs, SEEK_SET);
		fwrite(&count, sizeof(count), 1, resf);
		sortindex(1);
		fwrite(index, sizeof(TDataIndex) * count, 1, resf);
	}
	fclose(resf);
	if (count != 0) free(index);
}

