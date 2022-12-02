/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zaabou <zaabou@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/12/02 23:41:41 by zaabou            #+#    #+#             */
/*   Updated: 2022/12/02 23:41:42 by zaabou           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <libc.h>
#include <errno.h>

typedef	struct	process{
	char	**cmd;
	char	**env;
	int		fd_out;
	int		fd_in;
	char	type;
	int		end;
	int		un_pipe;
	struct process	*right;
	struct process	*left;
}t_p;

t_p	*creat_node(char type, char *env[])
{
	t_p	*node;

	node = malloc(1 * sizeof(t_p));
	if (node == NULL)
	{
		write (2, "error: fatal\n", 15);
		exit(1);
	}
	node->right = NULL;
	node->cmd = NULL;
	node->left = NULL;
	node->fd_out = 1;
	node->fd_in = 0;
	node->un_pipe = -1;
	node->end = 0;
	node->env = env;
	node->type = type;
	return (node);
}

void	get_next_token(char **av, int *index, t_p *cmd_node)
{
	while (av[*index])
	{
		if (strcmp(av[*index], "|") == 0 || strcmp(av[*index], ";") == 0)
			break ;
		(*index)++;
		cmd_node->end++;
	};
}

t_p	*creat_tree(char *av[], char *env[], int *index)
{
	t_p	*pipe_node;
	t_p	*cmd_node;

	pipe_node = NULL;
	cmd_node = NULL;
	while (av[*index] && strcmp(av[*index], ";") != 0)
	{
		if (strcmp(av[*index], "|") == 0)
		{
			pipe_node = creat_node('p', env);
			pipe_node->left = cmd_node;
			*index += 1;
			pipe_node->right = creat_tree(av, env, index);
			break ;
		}
		else
			cmd_node = creat_node('c', env);
		cmd_node->cmd = &av[*index];
		get_next_token(av, index, cmd_node);
	}
	return (pipe_node != NULL ? pipe_node : cmd_node);
}

int	ft_strlen(char *str)
{
	int	index = 0;

	while (str[index])
			index++;
	return (index);
}

void	cd(char **path)
{
	if (*path == NULL || path[1] != NULL)
		write (2, "error: cd: bad arguments\n", 26);
	else if (chdir(*path) == -1)
	{
		write (2, "error: cd: cannot change directory to ", 39);
		write(2, *path, ft_strlen(*path));
		write (2, "\n", 1);
	}
}
void	execute_cmd(t_p *node)
{
	pid_t	pid;

	(node->cmd)[node->end] = NULL;
	if (strcmp((node->cmd)[0], "cd") == 0)
		cd(&node->cmd[1]);
	else
	{
		pid = fork();
		if (pid == -1)
		{
			write (2, "error: fatal\n", 15);
        	exit(1);
		}
		else if (pid == 0)
		{
			if (node->un_pipe != -1)
				close(node->un_pipe);
			dup2(node->fd_out, 1);
			dup2(node->fd_in, 0);
			if (execve((node->cmd)[0], node->cmd, node->env) == -1)
			{
				write(2, "error: cannot execute ", 22);
				write(2, (node->cmd)[0], ft_strlen((node->cmd)[0]));
				write(2, "\n", 1);
				exit(1);
			}
		}
		else
		{
			if (node->fd_out != 1)
				close(node->fd_out);
			if (node->fd_in != 0)
				close(node->fd_in);
		}
	}
}

void	creat_pipe(t_p *head)
{
	int	fd[2];

	if (pipe(fd) == -1)
	{
		write (2, "error: fatal\n", 15);
        exit(1);
	}
	if (head->fd_in != 0)
		head->left->fd_in = head->fd_in;	
	head->left->fd_out = fd[1];
	head->left->un_pipe = fd[0];
	head->right->fd_in = fd[0];
	head->right->un_pipe = fd[1];
}

void	execution(t_p *head)
{
	if (head == NULL)
		return ;
	else if (head->type == 'p')
		creat_pipe(head);
	else if (head->type == 'c')
		execute_cmd(head);
	execution(head->left);
	execution(head->right);
}

void	wait_for_childs()
{
	while (waitpid(-1, NULL, 0) != -1);
}

void	free_tree(t_p *head)
{
	if (head == NULL)
		return ;
	free (head);
	free_tree(head->left);
	free_tree(head->right);
}

void	pipe_line(char **av, char **env, int *index)
{
	t_p	*head;

	head = creat_tree(av, env, index);
	execution(head);
	wait_for_childs();
	free_tree(head);
}
int	main(int ac, char *av[], char *env[])
{
	int	index;
	if (ac >= 2)
	{
		index = 1;
		while (++index < ac)
			pipe_line(av, env, &index);
	}
}
