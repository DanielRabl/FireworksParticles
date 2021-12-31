#include <qpl/qpl.hpp>

namespace info {
	constexpr qpl::f64 gravity = 200.0;
}

struct particle {
	qsf::circle circle;
	qpl::vector2f position;
	qpl::vector2f velocity;
	qpl::f64 life_time = 1.0;
	qpl::f64 radius;
	qpl::small_clock clock;
	bool active = true;
	qsf::rgb start_color = qsf::rgb::white;
	qsf::rgb end_color = qsf::rgb::white;

	
	void update_radius(qpl::f64 percentage) {
		this->circle.set_radius(this->radius * percentage);
		this->circle.centerize_origin();
	}
	void update_color(qpl::f64 percentage) {
		this->circle.set_color(this->start_color.interpolated(this->end_color, percentage));
	}
	void set_radius(qpl::f64 radius) {
		this->radius = radius;
		this->update_radius(1.0);
	}

	void update(const qsf::event_info& event) {
		if (!this->active) {
			return;
		}
		this->position += this->velocity * event.frame_time_f();
		this->velocity += qpl::vec(0, info::gravity * event.frame_time_f());

		this->circle.set_position(this->position);
		if (this->clock.has_elapsed(this->life_time)) {
			this->active = false;
			return;
		}
		auto f = this->clock.elapsed_f() / life_time;
		this->update_radius(1 - f);
		this->update_color(1 - f);
	}

	void draw(qsf::draw_object& draw) const {
		if (!this->active) {
			return;
		}
		draw.draw(this->circle);
	}
};

struct explosion {
	std::vector<particle> particles;
	qpl::vector2f position;
	qpl::size respawn;
	qpl::f64 respawn_time;
	qpl::clock respawn_clock;
	qsf::rgb start_color = qsf::rgb::unset;
	qsf::rgb end_color = qsf::rgb::unset;
	qsf::sprite sprite;
	qpl::clock light_clock;
	qpl::vector2f sprite_velocity;
	qpl::f64 color_n;

	explosion() {
		this->clear();
	}
	
	void set_texture(const sf::Texture& texture) {
		this->sprite.set_texture(texture);
		this->sprite.set_scale(1.1);
	}
	void set_position(qpl::vector2f position) {
		this->position = position;
		this->sprite.set_center(position);
	}
	qpl::size particle_size() const {
		return this->particles.size();
	}
	bool active() const {
		for (auto& i : this->particles) {
			if (i.active) {
				return true;
			}
		}
		return false;
	}
	void clear() {
		this->particles.clear();
		this->light_clock.reset();
		this->respawn_clock.reset();
		this->start_color = qsf::rgb::unset;
		this->end_color = qsf::rgb::unset;
		this->sprite_velocity = qpl::vec(0, 0);
	}
	void spawn(qpl::size size, qpl::f64 velocity, qpl::size respawn, qpl::f64 respawn_time, qpl::f64 life_time, qpl::f64 radius) {
		this->respawn = respawn;
		this->respawn_time = respawn_time;
		auto s = this->particles.size();
		this->particles.resize(s + size);

		this->light_clock.reset();
		if (this->start_color.is_unset()) {
			this->start_color = qsf::get_rainbow_color(this->color_n);
			this->start_color.interpolate(qsf::rgb::white, 0.3);
			this->end_color = qsf::get_rainbow_color(std::fmod(this->color_n + 0.2, 1.0));

			this->sprite.set_color(qsf::get_rainbow_color(this->color_n));
		}
		for (qpl::size i = s; i < s + size; ++i) {
			auto progress = (i - s) / qpl::f64_cast(size);
			auto angle = progress * 2 * qpl::pi;
			auto x = std::cos(angle);
			auto y = std::sin(angle);
			this->particles[i].set_radius(radius);
			this->particles[i].life_time = life_time;
			this->particles[i].start_color = this->start_color;
			this->particles[i].end_color = this->end_color;
			this->particles[i].position = this->position;
			this->particles[i].velocity = qpl::vec(x, y) * velocity;
		}
	}
	void spawn() {
		auto size = qpl::random(5, 70);
		auto velocity = qpl::random(100, 350);
		auto respawn = qpl::random(1, 4);
		auto respawn_time = qpl::random(0.005, 0.03);
		auto life_time = qpl::random(0.5, 1.3);
		auto radius = qpl::random(2.0, 6.0);
		if (size < 10) {
			radius *= qpl::random(1.0, 1.5);
		}
		this->spawn(size, velocity, respawn, respawn_time, life_time, radius);
	}
	void update(const qsf::event_info& event) {
		if (this->respawn && this->respawn_clock.has_elapsed_reset(this->respawn_time)) {
			this->spawn();
			--this->respawn;
		}
		event.update(this->particles);

		auto f = qpl::clamp_0_1(this->light_clock.elapsed_f() / 0.75);
		auto color = this->sprite.get_color();
		this->sprite.set_color(color.with_alpha(100 * (1 - f)));
		this->sprite_velocity.y += info::gravity * event.frame_time_f();
		this->sprite.set_position(this->sprite.get_position() + qpl::vec(0, this->sprite_velocity.y * event.frame_time_f()));
	}

	void draw(qsf::draw_object& draw) const {
		if (this->active()) {
			draw.draw(this->particles);
			draw.draw(this->sprite);
		}
	}
};

struct rocket {
	qsf::circle circle;
	qpl::vector2f position;
	qpl::vector2f velocity;
	qpl::f64 color_n;
	bool active = true;

	rocket() {
		this->set_radius(3);
	}
	void set_radius(qpl::f64 radius) {
		this->circle.set_radius(radius);
		this->circle.centerize_origin();
	}
	void randomize(qpl::f64 color_n, qpl::f64 width, qpl::f64 height) {
		this->set_position(qpl::vec(qpl::random(100.0, width - 100), height));
		this->velocity.x = qpl::random(-100, 100);
		this->velocity.y = qpl::random(-500, -300);
		this->color_n = color_n;
		auto color = qsf::get_rainbow_color(this->color_n);
		color.interpolate(qsf::rgb::white, 0.3);
		this->circle.set_color(color);
		this->active = true;
	}
	void set_position(qpl::vector2f pos) {
		this->position = pos;
		this->circle.set_position(this->position);
	}

	bool update(const qsf::event_info& event) {
		if (!this->active) {
			return false;
		}
		this->position += this->velocity * event.frame_time_f();
		this->velocity += qpl::vec(0, info::gravity * event.frame_time_f());

		auto f = 1 - qpl::clamp_0_1(-this->velocity.y / 300);
		this->set_radius(1.0 + 5.0 * f);

		this->circle.set_position(this->position);
		if (this->velocity.y >= 0) {
			this->active = false;
			return true;
		}
		return false;
	}
	
	void draw(qsf::draw_object& draw) const {
		if (!this->active) {
			return;
		}
		draw.draw(this->circle);
	}
};

struct main_state : qsf::base_state {
	void init() override {
		this->call_on_resize();
		this->spawn_gen.set_random_range(0.03, 0.45);
	}
	void call_on_resize() override {
		this->background.set_dimension(this->dimension());
	}
	void updating() override {
		static bool b = true;
		while (b) {
			this->framework->internal_update();
			if (this->event().key_pressed(sf::Keyboard::Space)) {
				b = false; 
			}
		}

		auto color_n = std::fmod(this->run_time().secs_f() / 20 + 0.5, 1.0);

		this->spawn_gen.update(this->frame_time_f());
		if (this->spawn_clock.has_elapsed_reset(this->spawn_gen.get())) {
			bool found = false;
			for (auto& i : this->rockets) {
				if (!i.active) {
					i.randomize(std::fmod(color_n + qpl::random(-0.1, 0.1), 1.0), this->dimension().x, this->dimension().y);
					found = true;
					break;
				}
			}
			if (!found) {
				this->rockets.push_back({});
				this->rockets.back().randomize(std::fmod(color_n + qpl::random(-0.1, 0.1), 1.0), this->dimension().x, this->dimension().y);
			}
		}

		for (auto& i : this->rockets) {
			if (i.update(this->event())) {

				bool found = false;
				for (auto& e : this->explosions) {
					if (!e.active()) {
						e.clear();
						e.color_n = i.color_n;
						e.set_position(i.position);
						e.spawn();
						found = true;
						break;
					}
				}
				if (!found) {
					this->explosions.push_back({});
					this->explosions.back().set_texture(this->get_texture("light"));
					this->explosions.back().color_n = i.color_n;
					this->explosions.back().set_position(i.position);
					this->explosions.back().spawn();
				}
			}
		}
		this->update(this->explosions);
	}
	void drawing() override {
		//this->draw(this->background);
		this->draw(this->rockets);
		this->draw(this->explosions);
	}
	qsf::rectangle background;
	std::vector<rocket> rockets;
	std::vector<explosion> explosions;
	qpl::cubic_generator spawn_gen;
	qpl::clock spawn_clock;
};

int main() {
	qsf::framework framework;
	framework.set_title("Fireworks");
	framework.set_dimension({ 1400u, 950u });
	framework.add_texture("light", "resources/light512.png");

	framework.add_state<main_state>();
	framework.game_loop();
}