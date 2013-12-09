#include <assert.h>
#include <stdlib.h>

#define STACK_MAX 256

/* Our language has two types, INT and PAIR. PAIR can contain more pairs
   or ints. Yay! */
typedef enum {
  OBJ_INT,
  OBJ_PAIR
} ObjectType;

/* Our language's Object. */
typedef struct sObject {
  /* What type of object is this? */
  ObjectType type;

  /* Marked for collection */
  unsigned char marked;

  /* The VM keeps it's own reference to objects that are distinct
     from the semantics that are visible to the language user. This field
     allows us to make the Object a node in the VM's linked list of Objects. */
  struct sObject* next;

  /* a union to hold the data for the int or pair. If your C is rusty, a
     union is a struct where the fields overlap in memory. Since a given
     object can only be an int or a pair, there’s no reason to have memory in
     a single object for all three fields at the same time. A union does
     that. Groovy. */
  union {
    int value;

    struct {
      struct sObject* head;
      struct sObject* tail;
    };
  };
} Object;

/* Our Virtual Machine */
typedef struct {
  /* The stack */
  Object* stack[STACK_MAX];
  
  // The current size of the stack.
  int stackSize;
  /* The total number of currently allocated objects. */
  int numObjects;
  /* The number of objects required to trigger a GC. */
  int maxObjects;

  /* The first object in the list of objects we've allocated, as described
     in the Object's definition above. */
  Object* firstObject;
} VM;

VM* newVM() {
  VM* vm = (VM *)malloc(sizeof(VM));
  vm->stackSize = 0;
  return vm;
}

void mark(Object* object) {
  
  /* Return if we've already marked this one. This prevents cycles and thereby
     explosions due to running forever. */
  if(object->marked) return;

  /* Mark the object as reachable. */
  object->marked = 1;

  /* If the object is a pair, its two fields are reachable too. */
  if(object->type == OBJ_PAIR) {
    mark(object->head);
    mark(object->tail);
  }
}

/* To mark all of the reachable objects, we start with the variables that are
   in memory, so that means walking the stack. Using mark() this will
   mark every object in memory that is reachable. */
void markAll(VM* vm) {
  for(int i = 0; i < vm->stackSize; i++) {
    mark(vm->stack[i]);
  }
}

/* Traverse the list of allocated objects and free any of them that
   aren't marked. */
void sweep(VM* vm) {
  Object** object = &vm->firstObject;
  while(*object) {
    if(!(*object)->marked) {
      /* This object wasn't reached, so remove it from the list and free it. */
      Object* unreached = *object;

      *object = unreached->next;
      free(unreached);
      /* Decrement the count of objects */
      vm->numObjects--;
    } else {
      /* This object was reached, so unmark it (for the next GC) and move on
         to the next. */
      (*object)->marked = 0;
      object = &(*object)->next;
    }
  }
}

/* Perform a garbage collection. */
void gc(VM* vm) {
  int numObjects = vm->numObjects;

  /* Mark… */
  markAll(vm);
  /* Sweep! */
  sweep(vm);

  /* Change the threshold for the next collection to 2 times the new
     number of objects. */
  vm->maxObjects = vm->numObjects * 2;
}

/* Function for adding an object from the stack. */
void push(VM* vm, Object* value) {
  assert(vm->stackSize < STACK_MAX); // Stack overflow
  vm->stack[vm->stackSize++] = value;
}

/* Function for removing an object from the stack. */
Object* pop(VM* vm) {
  assert(vm->stackSize > 0); // Stack underflow
  return vm->stack[--vm->stackSize];
}

/* Function for allocating a new object into the stack. */
Object* newObject(VM* vm, ObjectType type) {

  /* If the number of objects in the stack is equal to the max objects
     threshold, run the garbage collector. */
  if(vm->numObjects == vm->maxObjects) {
    gc(vm);
  }

  /* Allocate the new object */
  Object* object = malloc(sizeof(Object));
  /* Set it's type. */
  object->type = type;
  /* Init marked to zero. */
  object->marked = 0;

  /* Insert it to the list of allocated objects. */
  object->next = vm->firstObject;
  vm->firstObject = object;

  /* Increment the object count */
  vm->numObjects++;
  /* Return the object back to our caller. */
  return object;
}

void pushInt(VM* vm, int intValue) {
  Object* object = newObject(vm, OBJ_INT);
  object->value = intValue;
  push(vm, object);
}

Object* pushPair(VM* vm) {
  Object* object = newObject(vm, OBJ_PAIR);
  object->tail = pop(vm);
  object->head = pop(vm);

  push(vm, object);
  return object;
}

int main() {
  return 0;
}
