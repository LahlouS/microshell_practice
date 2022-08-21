#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#define FATAL "error: fatal\n"
#define BAD_CMD "error: cannot execute "
#define BAD_PATH "error: cd: cannot change directory to "
#define NO_PATH "error: cd: bad arguments\n"


#define END 0
#define PIPE 1
#define BREAK 2

struct t_data
{
    int fds[2];
    char **args;
    int len;
    char type;
    struct t_data *next;
    struct t_data *prev;
};

int ft_strlen(char *str)
{
    int i = 0;

    while (str[i])
        i++;
    return (i);
}

void    __free_data(struct t_data *data)
{
    int i = 0;
    struct t_data *tmp = data;

    while(data)
    {
        i = 0;
        if (data->args)
        {
            while (*(data->args + i))
                free(*(data->args + (i++)));
            free(*(data->args + i));
            free(data->args);
        }
        data = data->next;
        free(tmp);
        tmp = data;
    }
}

int __print_error(char *str1, char *str2, char *str3, int code)
{
    if (str1)
        write(2, str1, ft_strlen(str1));
    if (str2)
        write(2, str2, ft_strlen(str2));
    if (str3)
        write(2, str3, ft_strlen(str3));
    return (code);
}

char    *ft_strdup(char *str)
{
    int str_len = ft_strlen(str);
    char *ret = malloc(sizeof(char) * (str_len + 1));
    if (!ret)
        exit(__print_error(FATAL, NULL, NULL, 1));
    while (*str)
        *(ret++) = *(str++);
    *ret = '\0';
    return (ret - str_len);
}

int __add_arg(struct t_data *current, char *arg)
{
    char **tmp = malloc(sizeof(char *) * (current->len + 2));
    int i = 0;
    if (!tmp)
        exit(__print_error(FATAL, NULL, NULL, 1));
    while (current->args && current->args[i])
    {
        *(tmp + i) = *((current->args) + i);
        i++;
    }
    *(tmp + i) = ft_strdup(arg);
    i++;
    *(tmp + i) = NULL;
    if (current->args)
        free(current->args);
    current->args = tmp;
    current->len++;
    return (0);
}

int __add_node(struct t_data **data, struct t_data *current, char *arg)
{
    struct t_data *nouveau = malloc(sizeof(struct t_data) * 1);
    if (!nouveau)
        exit(__print_error(FATAL, NULL, NULL, 1));

    nouveau->type = END;
    nouveau->args = NULL;
    nouveau->len = 0;
    nouveau->next = NULL;
    nouveau->prev = NULL;
    if (!(*data))
        *data = nouveau;
    else
    {
        current->next = nouveau;
        nouveau->prev = current;
    }
    current = nouveau;
    __add_arg(current, arg);
    return (0);
}

int __parse_arg(struct t_data **data, char *arg)
{
    int is_colon = (strcmp(";", arg) == 0);
    int is_pipe = (strcmp("|", arg) == 0);
    struct t_data *current = *data;

    while (current && current->next)
        current = current->next;
    if (is_colon && !current)
        return (0);
    else if (!is_colon && !is_pipe && (!current || current->type != END))
        __add_node(data, current, arg);
    else if (is_colon)
        current->type = BREAK;
    else if (is_pipe)
        current->type = PIPE;
    else
        __add_arg(current, arg);
    return (0);
}

int __exec_cmd2(struct t_data *current, char **env)
{
    int ret = 0;
    int pid = 0;
    int status = 0;

    if ((current && current->type == PIPE) || (current->prev && current->prev->type == PIPE))
        if (pipe(current->fds) != 0)
            exit(__print_error(FATAL, NULL, NULL, 1));
    pid = fork();
    if (pid < 0)
         exit(__print_error(FATAL, NULL, NULL, 1));
    else if (pid == 0)
    {
        if (current->prev && current->prev->type == PIPE)
            if (dup2(current->prev->fds[0], 0) == -1)
                exit (__print_error(FATAL, NULL, NULL, 1));
        if (current && current->type == PIPE)
            if (dup2(current->fds[1], 1) == -1)
                exit (__print_error(FATAL, NULL, NULL, 1));
        if (execve(current->args[0], current->args, env) < 0)
             exit (__print_error(BAD_CMD, current->args[0], "\n", 1));
        exit(1);
    }
    else
    {
        waitpid(pid, &status, 0);
        if (current && current->type == PIPE)
            if (close(current->fds[1]) < 0)
                exit(__print_error(FATAL, NULL, NULL, 1));
        if (current->prev && current->prev->type == PIPE)
        {
            if (close(current->prev->fds[0]) < 0)
                exit(__print_error(FATAL, NULL, NULL, 1));   
            if (current && (current->type == BREAK || current->next == NULL))
            {
                if (close(current->fds[0]) < 0)
                    exit(__print_error(FATAL, NULL, NULL, 1));
                if (close(current->fds[1]) < 0)
                    exit(__print_error(FATAL, NULL, NULL, 1));
            }
        }
       if (WIFEXITED(status))
        ret = WIFEXITED(status);
    }
    return (ret);
}

int __exec_cmd1(struct t_data *current, char **env)
{
    if (strcmp("cd", current->args[0]) == 0)
    {
        if (current->len != 2)
            return (__print_error(NO_PATH, NULL, NULL, 1));
        if (chdir(current->args[1]) == -1)
            return (__print_error(BAD_PATH, current->args[1], "\n", 1));
    }
    else
        return (__exec_cmd2(current, env));
    return (1);
}

int main(int ac, char **av, char **env)
{
    struct t_data *data = NULL;
    struct t_data *tmp = NULL;
    int i = 0;
    int ret = 0;

    while (++i < ac)
        __parse_arg(&data, *(av + i));
    tmp = data;
    while (tmp)
    {
        ret = __exec_cmd1(tmp, env);
        tmp = tmp->next;
    }
    __free_data(data);
    return (ret);
}

    // while (tmp)
    // {
    //     printf("****AFFICHAGE*******\n-->   type = %d\n-->   args[0] = %s\n-->   len = %d\n\n\n", tmp->type, tmp->args[0], tmp->len);
    //     tmp = tmp->next;
    // }