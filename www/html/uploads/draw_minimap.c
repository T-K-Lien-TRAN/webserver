/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   draw_minimap.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajolivie <ajolivie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 10:00:44 by ajolivie          #+#    #+#             */
/*   Updated: 2025/07/24 10:07:04 by ajolivie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cub3d.h"

void	draw_minimap(t_settings *st)
{
	int	y;
	int	x;
	int	dx;
	int	dy;
	int	player_x;
	int	player_y;
	int	offset;

	y = 0;
	while (y < st->scheme->height)
	{
		x = 0;
		while (x < st->scheme->map[y].length)
		{
			if (st->scheme->map[y].path[x] == '1')
				draw_minimap_tile(y, x, st, 0x00FF00); // mur
			else
				draw_minimap_tile(y, x, st, 0x000000); // sol
			x++;
		}
		y++;
	}
	player_x = st->player.px * TILE_SIZE * MINIMAP_SCALE;
	player_y = st->player.py * TILE_SIZE * MINIMAP_SCALE;
	dy = -2;
	while (dy <= 2)
	{
		dx = -2;
		while (dx <= 2)
		{
			offset = ((player_y + dy) * st->addr.line_len)
				+ ((player_x + dx) * (st->addr.bpp / 8));
			*(unsigned int *)(st->img_data + offset) = 0xFF0000;
			dx++;
		}
		dy++;
	}
}

void	draw_minimap_tile(int map_y, int map_x, t_settings *st,
			unsigned int color)
{
	int	py;
	int	px;
	int	screen_x;
	int	screen_y;
	int	offset;

	py = 0;
	while (py < (int)(TILE_SIZE * MINIMAP_SCALE))
	{
		px = 0;
		while (px < (int)(TILE_SIZE * MINIMAP_SCALE))
		{
			screen_x = map_x * TILE_SIZE * MINIMAP_SCALE + px;
			screen_y = map_y * TILE_SIZE * MINIMAP_SCALE + py;
			offset = (screen_y * st->addr.line_len)
				+ (screen_x * (st->addr.bpp / 8));
			*(unsigned int *)(st->img_data + offset) = color;
			px++;
		}
		py++;
	}
}
