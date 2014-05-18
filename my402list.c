#include <stdlib.h>
#include "my402list.h"
#include<stdio.h>
    
int  My402ListLength(My402List *list){
        if(list != NULL)
	        return list -> num_members;
	    else
	    	return 0;
}

int  My402ListEmpty(My402List *list){
	if(list != NULL){
		if(list -> num_members == 0)
	        return 1;
	    else
	        return 0;
	}
	else
		return 0;
    
}

int  My402ListAppend(My402List *list, void *obj){
	if(list != NULL){
		My402ListElem *last = My402ListLast(list);
		My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem == 0 || new_elem == NULL){
			return 0;
		}
		else{    
		    new_elem -> obj = obj;
		    new_elem -> prev = last;
		    new_elem -> next = last -> next;
		    last -> next = new_elem;
		    list -> anchor.prev = new_elem;
		    list -> num_members++;
		    return 1;
		
		}
	}
	else
		return 0;
}

int  My402ListPrepend(My402List *list, void *obj){
    if(list != NULL){
    	My402ListElem *first = My402ListFirst(list);
		My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem == 0 || new_elem == NULL){
		    return 0;
		}
		else{
		    new_elem -> obj = obj;
		    new_elem -> next = first;
		    new_elem -> prev = first -> prev;
		    first -> prev = new_elem;
		    list -> anchor.next = new_elem;
		    list -> num_members++;
		    return 1;
		}
    }
    else
    	return 0;
}

void My402ListUnlink(My402List *list, My402ListElem *elem){
	if(list != NULL && elem != NULL)
	{
		elem -> prev -> next = elem -> next;
		elem -> next -> prev = elem -> prev;
		list -> num_members--;
		free(elem);
	}
}	

void My402ListUnlinkAll(My402List *list){
	if(list != NULL){
		My402ListElem *i = list -> anchor.next;
		My402ListElem *temp = NULL;
		while(i != list -> anchor.prev){
		    temp = i;
		    i = i -> next;
		    free(temp);
		}
		free(i);
		My402ListInit(list);
	}
}

int  My402ListInsertAfter(My402List *list, void *obj, My402ListElem *elem){
    if(list != NULL){
		if(elem == NULL){
		    if(My402ListAppend(list, obj))
		        return 1;
		    else
		        return 0;
		}
		else{
		    My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		    if(new_elem == 0 || new_elem == NULL){
		        return 0;
		    }   
		    else{
		        new_elem -> obj = obj;
		        elem -> next -> prev = new_elem;
		        new_elem -> next = elem -> next;
		        elem -> next = new_elem;
		        new_elem -> prev = elem;
		        list -> num_members++;
		        return 1;
		    }
		}
	}
	else
    	return 0;
}

int  My402ListInsertBefore(My402List *list, void *obj, My402ListElem *elem){
    if(list != NULL){
		if(elem == NULL){
		    if(My402ListPrepend(list, obj))
		        return 1;
		    else
		        return 0;
		}
		else{
			My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		    if(new_elem == 0 || new_elem == NULL){
		        return 0;
		    }   
		    else{
		        new_elem -> obj = obj;
		        elem -> prev -> next = new_elem;
		        new_elem -> prev = elem -> prev;
		        new_elem -> next = elem;
		        elem -> prev = new_elem;
		        list -> num_members++;
		        return 1;
		    }    
		}
	}
	else
    	return 0;
}

My402ListElem* My402ListFirst(My402List *list){
    if(list != NULL){
    	return list -> anchor.next;
    }
    else
    	return 0;	
    
}

My402ListElem* My402ListLast(My402List *list){
    if(list != NULL){
    	return list -> anchor.prev;	
    }
    else
    	return 0;	   
}

My402ListElem* My402ListNext(My402List *list, My402ListElem *elem){
	if(list != NULL && elem != NULL){
		if(list -> anchor.prev == elem)
		    return 0;
		else
		    return elem -> next;
	}
	else
		return 0;
}

My402ListElem* My402ListPrev(My402List *list, My402ListElem *elem){
   if(list != NULL && elem != NULL){
	   if(list -> anchor.next == elem)
		    return 0;
		else
		    return elem -> prev; 
   	}
   	else
   		return 0;
}

My402ListElem* My402ListFind(My402List *list, void *obj){
	if(list != NULL){
		My402ListElem *i = NULL;
		for(i = My402ListFirst(list); i != NULL; i = My402ListNext(list, i)){
		    if(i -> obj == obj)
		        return i;
		}
	}
    return 0;
}

int My402ListInit(My402List *list){
    if(list != NULL){
    	list -> num_members = 0;
    	list -> anchor.obj = NULL;
    	list -> anchor.next = &(list -> anchor);
    	list -> anchor.prev = &(list -> anchor);
    	return 1;
    }
    return 0;
}
