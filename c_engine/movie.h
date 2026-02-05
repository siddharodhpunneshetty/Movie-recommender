/**
 * movie.h - Data Structures for Movie Recommender System
 *
 * This header defines:
 * 1. Movie structure for storing movie information
 * 2. Hash Table with separate chaining for O(1) movie lookup
 * 3. Knowledge Graph using adjacency list for semantic relationships
 */

#ifndef MOVIE_H
#define MOVIE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============== CONFIGURATION ==============
#define HASH_TABLE_SIZE 127      // Prime number for better distribution
#define MAX_MOVIES 600           // Maximum movies supported
#define MAX_TITLE_LEN 100        // Maximum title length
#define MAX_GENRE_LEN 50         // Maximum genre length
#define MAX_RELATION_TYPE_LEN 30 // Maximum relationship type length
#define MAX_RECOMMENDATIONS 20   // Maximum recommendations to return
#define RATING_THRESHOLD 0.5     // Rating difference for similarity

// ============== RELATIONSHIP TYPES ==============
#define REL_GENRE_SIMILAR "GENRE_SIMILAR"
#define REL_RATING_SIMILAR "RATING_SIMILAR"

// ============== MOVIE STRUCTURE ==============
typedef struct {
  int id;
  char title[MAX_TITLE_LEN];
  char genre[MAX_GENRE_LEN];
  float rating;
} Movie;

// ============== HASH TABLE STRUCTURES ==============
// Node for separate chaining (collision handling)
typedef struct HashNode {
  Movie movie;
  struct HashNode *next;
} HashNode;

// Hash Table structure
typedef struct {
  HashNode *buckets[HASH_TABLE_SIZE];
  int count;
} HashTable;

// ============== KNOWLEDGE GRAPH STRUCTURES ==============
// Edge in the knowledge graph (represents a relationship)
typedef struct GraphEdge {
  int neighbor_id;                           // ID of connected movie
  char relation_type[MAX_RELATION_TYPE_LEN]; // Type of relationship
  struct GraphEdge *next;                    // Next edge in adjacency list
} GraphEdge;

// Node in the knowledge graph (represents a movie)
typedef struct {
  int movie_id;
  GraphEdge *edges; // Head of adjacency list
  int edge_count;   // Number of relationships
} GraphNode;

// Knowledge Graph structure
typedef struct {
  GraphNode nodes[MAX_MOVIES];
  int node_count;
  int movie_id_to_index[MAX_MOVIES]; // Maps movie ID to node index
} KnowledgeGraph;

// ============== QUEUE FOR BFS ==============
typedef struct {
  int items[MAX_MOVIES];
  int front;
  int rear;
} Queue;

// ============== WEIGHTED SCORING STRUCTURES ==============
// Weight constants for relationship types
#define WEIGHT_GENRE_SIMILAR 5  // Points for genre similarity
#define WEIGHT_RATING_SIMILAR 3 // Points for rating similarity

/**
 * MovieScore - Structure for weighted scoring recommendation
 *
 * Used to accumulate scores for candidate movies and sort them
 * by total_score (primary) and rating (secondary).
 */
typedef struct {
  int movie_id;    // Movie ID
  int total_score; // Accumulated weighted score
  float rating;    // Movie rating (for tie-breaking)
} MovieScore;

// ============== HASH TABLE FUNCTIONS ==============

/**
 * hashFunction - Compute hash value for a movie ID
 * @param id: Movie ID to hash
 * @return: Hash table index
 *
 * Time Complexity: O(1)
 */
unsigned int hashFunction(int id) {
  return (unsigned int)(id % HASH_TABLE_SIZE);
}

/**
 * initHashTable - Initialize hash table
 * @param ht: Pointer to hash table
 *
 * Time Complexity: O(TABLE_SIZE)
 */
void initHashTable(HashTable *ht) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    ht->buckets[i] = NULL;
  }
  ht->count = 0;
}

/**
 * insertMovie - Insert a movie into the hash table
 * @param ht: Pointer to hash table
 * @param movie: Movie to insert
 * @return: 1 on success, 0 on failure
 *
 * Time Complexity: O(1) average case
 * Uses separate chaining for collision handling
 */
int insertMovie(HashTable *ht, Movie movie) {
  unsigned int index = hashFunction(movie.id);

  // Create new node
  HashNode *newNode = (HashNode *)malloc(sizeof(HashNode));
  if (newNode == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return 0;
  }

  // Copy movie data
  newNode->movie = movie;
  newNode->next = NULL;

  // Insert at head of chain (separate chaining)
  if (ht->buckets[index] == NULL) {
    ht->buckets[index] = newNode;
  } else {
    newNode->next = ht->buckets[index];
    ht->buckets[index] = newNode;
  }

  ht->count++;
  return 1;
}

/**
 * searchMovie - Search for a movie by ID
 * @param ht: Pointer to hash table
 * @param id: Movie ID to search
 * @return: Pointer to movie if found, NULL otherwise
 *
 * Time Complexity: O(1) average case, O(n) worst case
 */
Movie *searchMovie(HashTable *ht, int id) {
  unsigned int index = hashFunction(id);
  HashNode *current = ht->buckets[index];

  // Traverse the chain
  while (current != NULL) {
    if (current->movie.id == id) {
      return &(current->movie);
    }
    current = current->next;
  }

  return NULL; // Not found
}

/**
 * freeHashTable - Free all memory used by hash table
 * @param ht: Pointer to hash table
 */
void freeHashTable(HashTable *ht) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    HashNode *current = ht->buckets[i];
    while (current != NULL) {
      HashNode *temp = current;
      current = current->next;
      free(temp);
    }
    ht->buckets[i] = NULL;
  }
  ht->count = 0;
}

// ============== KNOWLEDGE GRAPH FUNCTIONS ==============

/**
 * initKnowledgeGraph - Initialize knowledge graph
 * @param kg: Pointer to knowledge graph
 */
void initKnowledgeGraph(KnowledgeGraph *kg) {
  kg->node_count = 0;
  for (int i = 0; i < MAX_MOVIES; i++) {
    kg->nodes[i].movie_id = -1;
    kg->nodes[i].edges = NULL;
    kg->nodes[i].edge_count = 0;
    kg->movie_id_to_index[i] = -1;
  }
}

/**
 * addMovieToGraph - Add a movie node to the knowledge graph
 * @param kg: Pointer to knowledge graph
 * @param movie_id: Movie ID to add
 * @return: Index of the node
 */
int addMovieToGraph(KnowledgeGraph *kg, int movie_id) {
  // Check if already exists
  if (kg->movie_id_to_index[movie_id] != -1) {
    return kg->movie_id_to_index[movie_id];
  }

  int index = kg->node_count;
  kg->nodes[index].movie_id = movie_id;
  kg->nodes[index].edges = NULL;
  kg->nodes[index].edge_count = 0;
  kg->movie_id_to_index[movie_id] = index;
  kg->node_count++;

  return index;
}

/**
 * addRelation - Add a semantic relationship between two movies
 * @param kg: Pointer to knowledge graph
 * @param movie1_id: First movie ID
 * @param movie2_id: Second movie ID
 * @param relationType: Type of relationship (e.g., "GENRE_SIMILAR")
 *
 * Time Complexity: O(1)
 * This creates edges in both directions (undirected relationship)
 */
void addRelation(KnowledgeGraph *kg, int movie1_id, int movie2_id,
                 const char *relationType) {
  int idx1 = kg->movie_id_to_index[movie1_id];
  int idx2 = kg->movie_id_to_index[movie2_id];

  if (idx1 == -1 || idx2 == -1)
    return;

  // Add edge from movie1 to movie2
  GraphEdge *edge1 = (GraphEdge *)malloc(sizeof(GraphEdge));
  if (edge1 == NULL)
    return;

  edge1->neighbor_id = movie2_id;
  strncpy(edge1->relation_type, relationType, MAX_RELATION_TYPE_LEN - 1);
  edge1->relation_type[MAX_RELATION_TYPE_LEN - 1] = '\0';
  edge1->next = kg->nodes[idx1].edges;
  kg->nodes[idx1].edges = edge1;
  kg->nodes[idx1].edge_count++;

  // Add edge from movie2 to movie1 (bidirectional)
  GraphEdge *edge2 = (GraphEdge *)malloc(sizeof(GraphEdge));
  if (edge2 == NULL)
    return;

  edge2->neighbor_id = movie1_id;
  strncpy(edge2->relation_type, relationType, MAX_RELATION_TYPE_LEN - 1);
  edge2->relation_type[MAX_RELATION_TYPE_LEN - 1] = '\0';
  edge2->next = kg->nodes[idx2].edges;
  kg->nodes[idx2].edges = edge2;
  kg->nodes[idx2].edge_count++;
}

/**
 * freeKnowledgeGraph - Free all memory used by knowledge graph
 * @param kg: Pointer to knowledge graph
 */
void freeKnowledgeGraph(KnowledgeGraph *kg) {
  for (int i = 0; i < kg->node_count; i++) {
    GraphEdge *current = kg->nodes[i].edges;
    while (current != NULL) {
      GraphEdge *temp = current;
      current = current->next;
      free(temp);
    }
    kg->nodes[i].edges = NULL;
  }
  kg->node_count = 0;
}

// ============== QUEUE FUNCTIONS FOR BFS ==============

void initQueue(Queue *q) {
  q->front = 0;
  q->rear = -1;
}

int isQueueEmpty(Queue *q) { return q->rear < q->front; }

void enqueue(Queue *q, int item) {
  q->rear++;
  q->items[q->rear] = item;
}

int dequeue(Queue *q) { return q->items[q->front++]; }

#endif // MOVIE_H
