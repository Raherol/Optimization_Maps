#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define R 6371

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

typedef struct
{
    unsigned long id; // Node identification
    char *name;
    double lat, lon;      // Node position
    unsigned short nsucc; // Number of node successors; i. e. length of successors
    unsigned long *successors;
    int g;
    double h;
    double f;
    int index;
    int parent_index;
} node;

typedef struct
{
    node *nodes;
    unsigned short size;
} queue;

unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes);
queue dequeue(queue original_queue);
queue enqueue(queue original_queue, node *nodes, unsigned long index);
double haversine(double lat1, double lon1, double lat2, double lon2);
double toRadians(double degree);

int main(int argc, char *argv[])
{
    clock_t start_time;
    unsigned long nnodes;

    start_time = clock();

    char binmapname[80];
    strcpy(binmapname, argv[1]);

    FILE *binmapfile;
    start_time = clock();

    binmapfile = fopen(binmapname, "rb");
    fread(&nnodes, sizeof(unsigned long), 1, binmapfile);

    node *nodes;

    nodes = (node *)malloc(nnodes * sizeof(node));

    if (nodes == NULL)
    {
        printf("Error when allocating the memory for the nodes\n");
        return 2;
    }
    fread(nodes, sizeof(node), nnodes, binmapfile);

    for (int i = 0; i < nnodes; ++i)
    {
        nodes[i].successors = (unsigned long *)malloc(sizeof(unsigned long) * nodes[i].nsucc);
        fread(nodes[i].successors, sizeof(unsigned long), nodes[i].nsucc, binmapfile);
    }

    fclose(binmapfile);

    printf("Total number of nodes is %ld\n", nnodes);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);

    node origin_node, target_node;
    int origin_index, target_index;
    char *ptr;

    // We take the origin and target nodes for the A* algorithm
    origin_index = searchNode(strtoul(argv[2], &ptr, 10), nodes, nnodes);
    origin_node = nodes[origin_index];

    target_index = searchNode(strtoul(argv[3], &ptr, 10), nodes, nnodes);
    target_node = nodes[target_index];

    // We create the queue
    queue priorityqueue;
    priorityqueue.size = 1;
    priorityqueue.nodes = (node *)malloc(sizeof(node));
    priorityqueue.nodes[0] = origin_node;

    // A* algorithm begins
    node current_node;
    unsigned long succ_index;

    while (priorityqueue.size != 0)
    {
        current_node = priorityqueue.nodes[0];
        priorityqueue = dequeue(priorityqueue); // The first element of the queue is taken out

        if (current_node.id == target_node.id)
        {
            break; // We finish if this node is the target one
        }

        if (current_node.nsucc == 0)
        {
            continue; // We skip if it does not have any successors
        }
        else
        {
            for (int i = 0; i < current_node.nsucc; i++) // For every successor
            {
                succ_index = current_node.successors[i];
                if (nodes[succ_index].g == -1) // If it has not been visited we add it to the priority queue
                {
                    nodes[succ_index].g = current_node.g + 1;
                    nodes[succ_index].h = sqrt((nodes[succ_index].lat - target_node.lat) * (nodes[succ_index].lat - target_node.lat) + (nodes[succ_index].lon - target_node.lon) * (nodes[succ_index].lon - target_node.lon));
                    nodes[succ_index].f = nodes[succ_index].g + nodes[succ_index].h;
                    nodes[succ_index].parent_index = current_node.index;
                    priorityqueue = enqueue(priorityqueue, nodes, succ_index);
                }
            }
        }
    }
    printf("Path was started from: %lu and depth %d\n", nodes[origin_index].id, nodes[origin_index].g);
    printf("Path arrived at: %lu and depth %d\n", nodes[target_index].id, nodes[target_index].g);

    node *finalpath;
    finalpath = (node *)malloc((nodes[target_index].g + 1) * sizeof(node));
    finalpath[nodes[target_index].g] = nodes[target_index];
    for (int i = nodes[target_index].g - 1; i >= 0; i--)
    {
        finalpath[i] = nodes[finalpath[i + 1].parent_index];
    }

    double total_distance = 0;
    for (int i = 0; i <= nodes[target_index].g; i++)
    {
        if (i != 0)
        {
            total_distance += haversine(finalpath[i].lat, finalpath[i].lon, finalpath[i - 1].lat, finalpath[i - 1].lon);
        }
    }

    FILE *pathtxt;

    pathtxt = fopen("finalpath.txt", "w");

    fprintf(pathtxt, "# Distance from %d to %d: %lf meters.\n", atoi(argv[2]), atoi(argv[3]), total_distance);
    fprintf(pathtxt, "# Optimal path:\n");

    double cumulative_distance = 0;
    for (int i = 0; i <= nodes[target_index].g; i++)
    {
        if (i != 0)
        {
            cumulative_distance += haversine(finalpath[i].lat, finalpath[i].lon, finalpath[i - 1].lat, finalpath[i - 1].lon);
        }
        fprintf(pathtxt, "Id = %lu | %lf | %lf | Dist = %lf\n", finalpath[i].id, finalpath[i].lat, finalpath[i].lon, cumulative_distance);
    }

    fclose(pathtxt);

    return 0;
}

queue enqueue(queue original_queue, node *nodes, unsigned long index)
{
    original_queue.size++;
    original_queue.nodes = realloc(original_queue.nodes, original_queue.size * sizeof(node));
    if (original_queue.size != 1)
    {
        for (int i = 0; i < original_queue.size; i++)
        {
            if (i == original_queue.size - 1)
            {
                original_queue.nodes[i] = nodes[index];
            }
            if (nodes[index].f < original_queue.nodes[i].f)
            {
                for (int j = original_queue.size - 1; j > i; j--)
                {
                    original_queue.nodes[j] = original_queue.nodes[j - 1];
                }
                original_queue.nodes[i] = nodes[index];
                break;
            }
        }
    }
    else
    {
        original_queue.nodes[0] = nodes[index];
    }
    return original_queue;
}

queue dequeue(queue original_queue)
{
    queue temp_queue;
    temp_queue.size = original_queue.size - 1;
    temp_queue.nodes = (node *)malloc(temp_queue.size * sizeof(node));
    for (int i = 1; i < original_queue.size; i++)
    {
        temp_queue.nodes[i - 1] = original_queue.nodes[i];
    }
    free(original_queue.nodes);
    original_queue.nodes = (node *)malloc(temp_queue.size * sizeof(node));
    for (int i = 0; i < temp_queue.size; i++)
    {
        original_queue.nodes[i] = temp_queue.nodes[i];
    }
    original_queue.size = temp_queue.size;
    free(temp_queue.nodes);
    return original_queue;
}

unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes)
{
    // we know that the nodes where numrically ordered by id, so we can do a binary search.
    unsigned long l = 0, r = nnodes - 1, m;
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (nodes[m].id == id)
            return m;
        if (nodes[m].id < id)
            l = m + 1;
        else
            r = m - 1;
    }

    // id not found, we return nnodes+1
    return nnodes + 1;
}

double toRadians(double degree)
{
    return degree * M_PI / 180.0;
}

// Haversine function to calculate distance between two points
double haversine(double lat1, double lon1, double lat2, double lon2)
{
    // Convert latitude and longitude from degrees to radians
    lat1 = toRadians(lat1);
    lon1 = toRadians(lon1);
    lat2 = toRadians(lat2);
    lon2 = toRadians(lon2);

    // Calculate differences in coordinates
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Haversine formula
    double a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Calculate the distance
    double distance = R * c * 1000;

    return distance;
}