/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Buffer Handler in Embedded C++ *************************/
/**********************************************************/
#pragma once

/* HAL Libraries ******************************************/

/* Dependencies *******************************************/
#include <stdlib.h>
#include <stdint.h>

/* Typedef & Structs **************************************/
typedef enum Copy_Direction {Normal_Direction, Reverse_Direction} Copy_Direction;

/* Defines & Macros ***************************************/

/* Class **************************************************/
template <typename T>
class buffer {
  public:
    /* Constructeur */
	buffer<T>(int size);
    /* API */
    bool push(T element);
    T pop();
    void copy(uint8_t *bufferToPush,int copylength, Copy_Direction direction);
    void clean();
    /* Flags */
    bool isEmpty();
    bool isFull();
    /* Accessor */
    T* getBufferPtr();
    int getSize();
    int getNumberOfElements();
    /* Dangerous editor */
    void setWriteCursor(int length);
    ~buffer();
  private:
    T *ptrBuffer;
    int size;
    /* buffer parameters */
    int writeCursor;
    int readCursor;
};

/* Inclusion de la source dans le header pour éviter les problèmes de link du compilateur */
template <typename T>
buffer<T>::buffer(int size){
  this->ptrBuffer = (T*) malloc((size)*sizeof(T));
  this->size = size;
  this->writeCursor = 0;
  this->readCursor = 0;
}

template <typename T>
bool buffer<T>::push(T element){
  if(this->writeCursor == (this->size)){return false;}
  this->ptrBuffer[this->writeCursor] = element;
  this->writeCursor++;
  return true;
}

template <typename T>
T buffer<T>::pop(){
  if( this->readCursor != this->writeCursor){
    T value = this->ptrBuffer[this->readCursor];
    this->readCursor++;
    return value;
  }
  else {return 0;}
}

template <typename T>
void buffer<T>::copy(uint8_t *bufferToPush,int copylength, Copy_Direction direction){
	if(direction == Normal_Direction){
		for(int i=0;i<copylength;i++){
			this->push(bufferToPush[i]);
		}
	}
	else if(direction == Reverse_Direction){
		for(int i=1;i<=copylength;i++){
			this->push(bufferToPush[copylength-i]);
		}
	}
}

template <typename T>
void buffer<T>::clean(){
  this->writeCursor = 0;
  this->readCursor = 0;
}

template <typename T>
bool buffer<T>::isEmpty(){return (this->writeCursor == 0);}

template <typename T>
bool buffer<T>::isFull(){return (this->writeCursor == this->size);}

template <typename T>
T * buffer<T>::getBufferPtr(){return this->ptrBuffer;}

template <typename T>
int buffer<T>::getSize(){return this->size;}

template <typename T>
int buffer<T>::getNumberOfElements(){return (this->writeCursor - this->readCursor);}

template <typename T>
void buffer<T>::setWriteCursor(int length){this->writeCursor += length;}

template <typename T>
buffer<T>::~buffer(){free(this->ptrBuffer);}
