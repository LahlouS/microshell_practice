#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define FATAL "error: fatal\n"
#define BAD_ARG "error: cd: bad arguments\n"
#define BAD_PATH "error: cd: cannot change directory to "
#define BAD_CMD "error: cannot execute "

#define PIPE 0
#define BREAK 1
#define END 2

typedef struct s_data
{
	char **args;
	int	fds[2];
	char type;
	int	len;
	struct s_data *next;
	struct s_data *prev;
}	t_data;

void __free_data(t_data *data)
{
	t_data *tmp = data;
	int i = 0;
	while (data)
	{
		tmp = data;
		if (data->args)
		{
			i = 0;
			while (*((data->args) + i))
			{
				free(*((data->args) + i));
				i++;
			}
			free(*((data->args) + i));
			free(data->args);
		}
		data = data->next;
		free(tmp);
	}
}

int	ft_strlen(char *str)
{
	int i = 0;
	while (str[i])
		i++;
	return (i);
}

int	__print_error(char *str1, char *str2, char *str3, int code)
{
	if (str1)
		write(2, str1, ft_strlen(str1));
	if (str2)
		write(2, str2, ft_strlen(str2));
	if (str3)
		write(2, str3, ft_strlen(str3));
	return (code);
}

char	*ft_strdup(char *to_dup)
{
	int i = 0;
	int str_len = ft_strlen(to_dup);
	char *tmp = malloc(sizeof(char) * (str_len + 1));
	if (!tmp)
		exit(__print_error(FATAL, NULL, NULL, 1));
	while (to_dup[i])
	{
		tmp[i] = to_dup[i];
		i++;
	}
	tmp[i] = 0;
	return (tmp);
}

int	__add_arg(t_data *nouveau, char *arg)
{
	int	i = 0;
	char **tmp = malloc(sizeof(char *) * (nouveau->len + 2));
	if (!tmp)
		exit(__print_error(FATAL, NULL, NULL, 1));
	while (nouveau->args && *((nouveau->args) + i))
	{
		*(tmp + i) = *((nouveau->args) + i);
		i++;
	}
	*(tmp + i) = ft_strdup(arg);
	i++;
	*(tmp + i) = NULL;
	printf("i = %d\n", i);
	if (nouveau->args)
		free(nouveau->args);
	nouveau->args = tmp;
	nouveau->len += 1;
	return (0);
}

void	__add_node(t_data **data, t_data *current, char *arg)
{
	t_data *nouveau = malloc(sizeof(t_data) * 1);
	if (!nouveau)
		exit(__print_error(FATAL, NULL, NULL, 1));
	nouveau->type = END;
	nouveau->len = 0;
	nouveau->args = NULL;
	nouveau->next = NULL;
	nouveau->prev = NULL;
	if (!(*data))
	{
		*data = nouveau;
	}
	else
	{
		current->next = nouveau;
		nouveau->prev = current;
	}
	__add_arg(nouveau, arg);
}

void	__parse_arg(t_data **data, char *arg)
{
	int	is_pipe = (strcmp(arg, "|") == 0);
	int	is_colon = (strcmp(arg, ";") == 0);
	t_data *tmp = *data;

	while (tmp && tmp->next)
		tmp = tmp->next;
	if (is_colon && tmp == NULL)
		return;
	else if(!is_colon && (tmp == NULL || tmp->type != END))
		__add_node(data, tmp, arg);
	else if (is_pipe && *data != NULL)
		tmp->type = PIPE;
	else if (is_colon)
		tmp->type = BREAK;
	else
		__add_arg(tmp, arg);
}

int	__exec_cmd2(t_data *current, char **env)
{
	int pid = 0;
	int status = 0;

	if ((current && current->type == PIPE) || (current->prev && current->prev->type == PIPE))
	{
		if (pipe(current->fds))
			exit(__print_error(FATAL, NULL, NULL, 1));
	}
	pid = fork();
	if (pid == 0)
	{
		if ((current->prev && current->prev->type == PIPE))
			if (dup2(current->prev->fds[0], 0) == -1)
				exit(__print_error(FATAL, NULL, NULL, 1));
		if (current && current->type == PIPE)
			if (dup2(current->fds[1], 1) == -1)
				exit(__print_error(FATAL, NULL, NULL, 1));
		if (execve(current->args[0], current->args, env) == -1)
			exit (__print_error(BAD_CMD, current->args[0], "\n", 1));
		return (1);
	}
	else
	{
		waitpid(pid, &status, 0);
		if ((current && current->type == PIPE))
			close(current->fds[1]);
		if (current->prev && current->prev->type == PIPE)
			close(current->prev->fds[0]);
		if ((current->next == NULL || current->type == BREAK) && (current->prev && current->prev->type == PIPE))
			close(current->fds[0]);
		if (WIFEXITED(status))
			return (WIFEXITED(status));
	}
	return (0);
}

int	__exec_cmd1(t_data *current, char **env)
{
	if (strcmp(current->args[0], "cd") == 0)
	{
		if (current->len < 2)
			return (__print_error(BAD_ARG, NULL, NULL, 1));
		if (chdir(current->args[1]))
			return (__print_error(BAD_PATH, current->args[1], "\n", 1));
	}
	else
		return (__exec_cmd2(current, env));
	return (0);
}

int main(int ac, char **av, char **env)
{
	int	i = 0;
	int	ret = 0;
	t_data	*data = NULL;
	t_data	*temp = NULL;
	(void)env;
	if (ac > 1)
	{
		while (++i < ac)
			__parse_arg(&data, av[i]);
		temp = data;
		while (temp)
		{
			printf("***AFFICHAGE***\n-->   type = %d\n-->   args[0] = %s\n-->   len = %d\n\n\n", temp->type, *(temp->args), temp->len);
			temp = temp->next;
		}
		temp = data;
		while (temp)
		{
			ret = __exec_cmd1(temp, env);
			temp = temp->next;
		}
		temp = data;
		while (temp)
		{
			printf("***AFFICHAGE PART 2***\n-->   type = %d\n-->   args[0] = %s\n-->   len = %d\n\n\n", temp->type, *(temp->args), temp->len);
			temp = temp->next;
		}
		__free_data(data);
	}
	return (ret);
}

