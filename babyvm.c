#include <assert.h>
#include <stdio.h>
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

  /* Marked for collection. Note that storing this in the object is a bad idea
     because GC will cause copy-on-write to go nuts in the event you are using
     fork(). It's good enough for this demo, though. */
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
  printf("\tMarking %d objects\n", vm->stackSize);
  for(int i = 0; i < vm->stackSize; i++) {
    mark(vm->stack[i]);
  }
}

/* Traverse the list of allocated objects and free any of them that
   aren't marked. */
void sweep(VM* vm) {
  Object** object = &vm->firstObject;
  int swept = 0;
  int freed = 0;
  while(*object) {
    if(!(*object)->marked) {
      /* This object wasn't reached, so remove it from the list and free it. */
      Object* unreached = *object;

      *object = unreached->next;
      free(unreached);
      /* Increment the number we've freed. */
      freed++;
      /* Decrement the count of objects */
      vm->numObjects--;
    } else {
      /* This object was reached, so unmark it (for the next GC) and move on
         to the next. */
      (*object)->marked = 0;
      object = &(*object)->next;
    }
    /* Increment the number we've swept. */
    swept++;
  }
  printf("\tSwept %d objects, freed %d.\n", swept, freed);
}

/* Perform a garbage collection. */
void gc(VM* vm) {
  printf("\nEntering GC\n");
  int numObjects = vm->numObjects;

  /* Mark… */
  markAll(vm);
  /* Sweep! */
  sweep(vm);

  /* Change the threshold for the next collection to 2 times the new
     number of objects. */
  vm->maxObjects = vm->numObjects * 2;
  printf("GC completed, Total objects now %d. Threshold is %d.\n\n", vm->numObjects, vm->maxObjects);
}

/* Function for adding an object from the stack. */
void push(VM* vm, Object* value) {
  assert(vm->stackSize < STACK_MAX); // Stack overflow
  vm->stack[vm->stackSize++] = value;
  printf("Adding object to stack, size is now %d.\n", vm->stackSize);
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
  printf("Checking for GC: %d objects in stack == %d max objects\n", vm->numObjects, vm->maxObjects);
  if(vm->numObjects == vm->maxObjects) {
    printf("GC needed\n");
    gc(vm);
  } else {
    printf("GC not needed\n");
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
  printf("Created object, number of objects is now %d\n", vm->numObjects);
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
  /* Create a new VM */
  VM* vm = newVM();
  vm->maxObjects = 1;

  printf("Adding integer 0 to the stack.\n");
  pushInt(vm, 0);

  printf("Adding integer 1 to the stack.\n");
  pushInt(vm, 1);

  printf("Adding a pair to the stack (consuming two ints already there).\n");
  pushPair(vm);

  printf("There are now %d objects in stack and %d objects have been allocated.\n", vm->stackSize, vm->numObjects);

  /* Remove it from the stack, simulating the variable no longer being referenced. */
  printf("Popping pair from the stack.\n");
  Object* o = pop(vm);

  printf("There are now %d objects in stack and %d objects have been allocated.\n", vm->stackSize, vm->numObjects);

  printf("Manual invoking GC (should free all)");
  gc(vm);

  /* Done with the VM! */
  free(vm);

  return 0;
}
