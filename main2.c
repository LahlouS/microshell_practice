#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


#define FATAL "error: fatal\n"
#define BAD_CMD "error: cannot execute "
#define BAD_PATH "error: cd: cannot change directory to "
#define NO_ARG_CD "error: cd: bad arguments\n"

#define PIPE 0
#define END 1
#define BREAK 2

typedef struct s_data
{
	int type;
	char **args;
	char len;
	int fds[2];
	struct s_data *prev;
	struct s_data *next;
} t_data;

int	ft_strlen(char *str)
{
	int i = 0;

	while (str[i])
		i++;
	return (i);
}

void	__free_data(t_data *data)
{
	t_data *tmp = data;
	int i = 0;

	while (data)
	{
		i = 0;
		close(data->fds[0]);
		close(data->fds[1]);
		if (data->args)
		{
			while (*(data->args + i))
			{
				free(*(data->args + i));
				i++;
			}
			free(*(data->args + i));
			free(data->args);
		}

		data = data->next;
		free(tmp);
		tmp = data;
	}
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

char	*ft_strdup(char *str)
{
	int str_len = ft_strlen(str);
	char	*to_return = malloc(sizeof(char) * (str_len + 1));
	int	i = 0;
	if (!to_return)
		exit (__print_error(FATAL, NULL, NULL, 1));
	while (str[i])
	{
		to_return[i] = str[i];
		i++;
	}
	to_return[i] = '\0';
	return (to_return);
}

int	__add_arg(t_data *current, char *arg)
{
	int i = 0;
	char	**temp = malloc(sizeof(char *) * (current->len + 2));
	if (!temp)
		exit(__print_error(FATAL, NULL, NULL, 1));
	while (current->args && *(current->args + i))
	{
		*(temp + i) = *(current->args + i);
		i++;
	}
	*(temp + i) = ft_strdup(arg);
	i++;
	*(temp + i) = NULL;
	if (current->args)
		free(current->args);
	current->args = temp;
	current->len += 1;

	return (0);
}

int	__add_node(t_data **data, t_data *current, char *arg)
{
	t_data	*nouveau = malloc(sizeof(t_data) * 1);
	if (!nouveau)
		exit (__print_error(FATAL, NULL, NULL, 1));
	nouveau->prev = NULL;
	nouveau->next = NULL;
	nouveau->args = NULL;
	nouveau->type = END;
	nouveau->len = 0;
	if (*data == NULL)
		*data = nouveau;
	else
	{
		nouveau->prev = current;
		current->next = nouveau;
	}
	current = nouveau;
	__add_arg(current, arg);
	return (0);
}

void	__parse_args(t_data **data, char *arg)
{
	int is_pipe = ((strcmp("|", arg)) == 0);
	int is_colon = ((strcmp(";", arg)) == 0);
	t_data *current = *data;

	while (current && current->next)
		current = current->next;
	if (is_colon && *data == NULL)
		return ;
	else if (is_pipe)
		current->type = PIPE;
	else if (is_colon)
		current->type = BREAK;
	else if (!is_colon && (current == NULL || current->type != END))
		__add_node(data, current, arg);
	else
		__add_arg(current, arg);
}

int	__exec_cmd2(t_data *current, char **env)
{
	int ret = 0;
	int pid = 0;
	int status = 0;

	if ((current && current->type == PIPE) || (current->prev && current->prev->type == PIPE))
		if (pipe(current->fds))
			exit(__print_error(FATAL, NULL, NULL, 1));
	pid = fork();
	if (pid < 0)
		exit(__print_error(FATAL, NULL, NULL, 1));
	else if (pid == 0)
	{
		if ((current && current->type == PIPE))
			if (dup2(current->fds[1], 1) < 0)
				exit(__print_error(FATAL, NULL, NULL, 1));
		if ((current->prev && current->prev->type == PIPE))
			if (dup2(current->prev->fds[0], 0) < 0)
				exit(__print_error(FATAL, NULL, NULL, 1));
		if (execve(current->args[0], current->args, env) < 0)
		{
			exit(__print_error(BAD_CMD, current->args[0], "\n", 1));
		}
	}
	else if (pid)
	{
		waitpid(pid, &status, 0);
		if ((current && current->type == PIPE))
			if (close(current->fds[1]) < 0)
				exit (__print_error(FATAL, NULL, NULL, 1));
		if ((current->prev && current->prev->type == PIPE))
		{
			if (close(current->prev->fds[0]) < 0)
				exit (__print_error(FATAL, NULL, NULL, 1));
			if (current->type == BREAK || current->next == NULL)
				if (close(current->fds[1]) < 0)
					exit (__print_error(FATAL, NULL, NULL, 1));
		}
		if (WIFEXITED(status))
			ret = WIFEXITED(status);
	}
	return (ret);
}

int	__exec_cmd1(t_data *current, char **env)
{
	if (strcmp("cd", current->args[0]) == 0)
	{
		if (current->len != 2)
			return (__print_error(NO_ARG_CD, NULL, NULL, 1));
		if (chdir(current->args[1]) < 0)
			return (__print_error(BAD_PATH, current->args[1], "\n", 1));
	}
	else
		return (__exec_cmd2(current, env));
	return (0);
}

int main(int ac, char **av, char **env)
{
	t_data	*data = NULL;
	t_data	*tmp = NULL;
	int		ret = 0;
	int		i = 0;

	while (++i < ac)
		__parse_args(&data, *(av + i));
	tmp = data;
	while (tmp)
	{
		ret = __exec_cmd1(tmp, env);
		tmp = tmp->next;
	}
	__free_data(data);
	return (ret);
}
/*
	tmp = data;
while (tmp)
	{
		printf("***********************\n-->   type = %d\n-->   arg[0] = %s\n-->   len = %d\n\n\n", tmp->type, tmp->args[0], tmp->len);
		tmp = tmp->next;
	}
*/

